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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICESESSION_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICESESSION_H_


#include <ICameraDevice3Session.h>
#include <HidlCameraDevice.h>
#include <HidlCameraCommon.h>
#include <IBufferHandleCacheMgr.h>

#include <mutex>
#include <memory>
#include <vector>

#include <utils/Mutex.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace hidl_dev3 {

/******************************************************************************
 *
 ******************************************************************************/
class HidlCameraDeviceSession
    : public ::android::hardware::camera::device::V3_6::ICameraDeviceSession
    , public ::android::hardware::hidl_death_recipient
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.
    virtual             ~HidlCameraDeviceSession();
                        HidlCameraDeviceSession(const ::android::sp<ICameraDevice3Session>& session);
    static auto         create(
                            const ::android::sp<ICameraDevice3Session>& session
                        ) -> HidlCameraDeviceSession*;

public:    ////        Operations.
    virtual auto        open(
                            const ::android::sp<V3_5::ICameraDeviceCallback>& callback
                        ) -> ::android::status_t;

private:    ////        Operations.
    virtual auto        initialize() -> ::android::status_t;
    virtual auto        getSafeBufferHandleCacheMgr() const -> ::android::sp<Utils::IBufferHandleCacheMgr>;
    auto const&         getLogPrefix() const            { return mLogPrefix; }

private:
    virtual auto        createSessionCallbacks() -> std::shared_ptr<v3::CameraDevice3SessionCallback>;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraDeviceSession Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    virtual Return<void> constructDefaultRequestSettings(RequestTemplate type, constructDefaultRequestSettings_cb _hidl_cb) override;
    virtual Return<void> configureStreams(const V3_2::StreamConfiguration& requestedConfiguration, configureStreams_cb _hidl_cb) override;
    virtual Return<void> getCaptureRequestMetadataQueue(getCaptureRequestMetadataQueue_cb _hidl_cb) override;
    virtual Return<void> getCaptureResultMetadataQueue(getCaptureResultMetadataQueue_cb _hidl_cb) override;
    virtual Return<void> processCaptureRequest(const hidl_vec<V3_2::CaptureRequest>& requests, const hidl_vec<BufferCache>& cachesToRemove, processCaptureRequest_cb _hidl_cb) override;
    virtual Return<Status> flush() override;
    virtual Return<void> close() override;

    //V3_4
    virtual Return<void> configureStreams_3_3(const V3_2::StreamConfiguration& requestedConfiguration, configureStreams_3_3_cb _hidl_cb) override;
    virtual Return<void> configureStreams_3_4(const V3_4::StreamConfiguration& requestedConfiguration, configureStreams_3_4_cb _hidl_cb) override;
    virtual Return<void> processCaptureRequest_3_4(const hidl_vec<V3_4::CaptureRequest>& requests, const hidl_vec<BufferCache>& cachesToRemove, processCaptureRequest_3_4_cb _hidl_cb) override;

    //V3_5
    virtual Return<void> configureStreams_3_5(const V3_5::StreamConfiguration& requestedConfiguration, configureStreams_3_5_cb _hidl_cb) override;
    virtual Return<void> signalStreamFlush(const hidl_vec<int32_t>& streamIds, uint32_t streamConfigCounter) override;
    virtual Return<void> isReconfigurationRequired(const hidl_vec<uint8_t>& oldSessionParams, const hidl_vec<uint8_t>& newSessionParams, isReconfigurationRequired_cb _hidl_cb) override;

    //V3_6
    virtual Return<void> configureStreams_3_6(const V3_5::StreamConfiguration& requestedConfiguration, configureStreams_3_6_cb _hidl_cb) override;
    virtual Return<void> switchToOffline(const hidl_vec<int32_t>& streamsToKeep, switchToOffline_cb _hidl_cb) override;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ::android::hardware::hidl_death_recipient
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    virtual void        serviceDied(uint64_t cookie, const android::wp<android::hidl::base::V1_0::IBase>& who) override;

private:
    virtual auto        processCaptureResult(std::vector<v3::CaptureResult>& results) -> void;

    virtual auto        notify(const std::vector<v3::NotifyMsg>& msgs) -> void;

    virtual auto        requestStreamBuffers(
                            const std::vector<v3::BufferRequest>& vBufferRequests,
                            std::vector<v3::StreamBufferRet>* pvBufferReturns
                        ) -> v3::BufferRequestStatus;

    virtual auto        returnStreamBuffers(const std::vector<v3::StreamBuffer>& buffers) -> void;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    mutable android::Mutex                      mCameraDeviceCallbackLock;
    ::android::sp<ICameraDevice3Session>   mSession = nullptr;
    ::android::sp<V3_5::ICameraDeviceCallback>          mCameraDeviceCallback = nullptr;

protected:
    std::mutex                                  mInterfaceLock;
    std::string const                           mLogPrefix;
    int32_t                                     mConvertTimeDebug;
    int32_t                                     mAutoTestDebug = false;
    bool                                        mIsClosed;

    // linkToDeath
    ::android::hidl::base::V1_0::DebugInfo      mLinkToDeathDebugInfo;

    mutable android::Mutex                      mBufferHandleCacheMgrLock;
    ::android::sp<Utils::IBufferHandleCacheMgr>        mBufferHandleCacheMgr;

protected:  ////                Metadata Fast Message Queue (FMQ)
    using RequestMetadataQueue = ::android::hardware::MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
    std::unique_ptr<RequestMetadataQueue>
                                mRequestMetadataQueue;

    using ResultMetadataQueue = ::android::hardware::MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
    std::unique_ptr<ResultMetadataQueue>
                                mResultMetadataQueue;

private:
    /**
     * This shared pointer hold CameraDevice3SessionCallback after it is created
     * in initialize(). User of CameraDevice3SessionCallback should use weak
     * pointer to link this structure. While HidlCameraDeviceSession is being
     * destructed, this shared pointer will be released and
     * CameraDevice3SessionCallback wil be freed.
     * Thus, user will not be able to get this structure with weak_ptr::lock().
     */
    std::shared_ptr<v3::CameraDevice3SessionCallback> mSessionCallbacks;

private:    ////        interface/structure conversion helper

    virtual auto        convertStreamConfigurationFromHidl(
                            const V3_5::StreamConfiguration& srcStreams,
                            NSCam::v3::StreamConfiguration& dstStreams
                        ) -> ::android::status_t;

    virtual auto        convertHalStreamConfigurationToHidl(
                            const NSCam::v3::HalStreamConfiguration& srcStreams,
                            V3_6::HalStreamConfiguration& dstStreams
                        ) -> ::android::status_t;

    virtual auto        convertAllRequestFromHidl(
                            const hidl_vec<V3_4::CaptureRequest>& inRequests,
                            RequestMetadataQueue* pRequestFMQ,
                            std::vector<NSCam::v3::CaptureRequest>& outRequests,
                            CameraMetadata& metadata_queue_settings,
                            std::vector<Utils::RequestBufferCache>* requestBufferCache
                        ) -> ::android::status_t;

    virtual auto        convertToHalCaptureRequest(
                            const V3_4::CaptureRequest& hidl_request,
                            RequestMetadataQueue* request_metadata_queue,
                            NSCam::v3::CaptureRequest* hal_request,
                            CameraMetadata& metadata_queue_settings,
                            Utils::RequestBufferCache* requestBufferCache
                        ) -> ::android::status_t;

    virtual auto        convertToHalMetadata(
                            uint32_t message_queue_setting_size,
                            RequestMetadataQueue* request_metadata_queue,
                            const CameraMetadata& request_settings,
                            CameraMetadata& metadata_queue_settings,
                            camera_metadata** hal_metadata
                        ) -> ::android::status_t;

    virtual auto        convertToHalStreamBuffer(
                            const StreamBuffer& hidl_buffer,
                            NSCam::v3::StreamBuffer* hal_buffer,
                            const Utils::BufferHandle* importedBufferHandle
                        ) -> ::android::status_t;

    virtual auto        convertToHidlCaptureResults(
                            const std::vector<v3::CaptureResult>& hal_captureResults,
                            std::vector<V3_4::CaptureResult>* hidl_captureResults
                        ) -> ::android::status_t;

    virtual auto        convertToHidlCaptureResult(
                            const v3::CaptureResult& hal_captureResult,
                            V3_4::CaptureResult* hidl_captureResult
                        ) -> ::android::status_t;

    virtual auto        convertToHidlStreamBuffer(
                            const NSCam::v3::StreamBuffer& hal_buffer,
                            V3_2::StreamBuffer* hidl_buffer,
                            uint32_t requestNo = -1
                        ) -> ::android::status_t;

    virtual auto        convertToHidlNotifyMsgs(
                            const std::vector<v3::NotifyMsg>& hal_messages,
                            std::vector<V3_2::NotifyMsg>* hidl_messages
                        ) -> ::android::status_t;

    virtual auto        convertToHidlErrorMessage(
                            const v3::ErrorMsg& hal_error,
                            V3_2::ErrorMsg* hidl_error
                        ) -> ::android::status_t;

    virtual auto        convertToHidlShutterMessage(
                            const v3::ShutterMsg& hal_shutter,
                            V3_2::ShutterMsg* hidl_shutter
                        ) -> ::android::status_t;

private :
    virtual auto        printAutoTestLog(
                            const camera_metadata_t* pMetadata,
                            bool bIsOutput
                        ) -> void;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace hidl_dev3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_HIDL_HIDLCAMERADEVICESESSION_H_

