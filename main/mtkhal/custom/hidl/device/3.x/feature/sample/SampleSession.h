/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_
#define MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_

#include <mtkcam-android/main/standalone_isp/NativeDev/INativeCallback.h>
#include <mtkcam-android/main/standalone_isp/NativeDev/INativeDev.h>
#include <mtkcam-android/main/standalone_isp/NativeDev/NativeModule.h>

#include <mtkcam/utils/metastore/IMetadataProvider.h>

#include <IFeatureSession.h>
#include <utils/Mutex.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../../graphics/AllocatorHelper.h"
#include "../../graphics/MapperHelper.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace custom_dev3 {
namespace feature {
namespace sample {

/******************************************************************************
 * A sample module derived from IFeatureSession to implement simple example for
 * MTK HAL3+ I/F to integrate data collection and isp reprocess flow.
 ******************************************************************************/
class SampleSession : public IFeatureSession {
  friend class NativeCallback;

 public:
  struct NativeCallback : public v3::NativeDev::INativeCallback {
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  INativeCallback
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   public:
    int32_t processCompleted(int32_t const& status,
                             v3::NativeDev::Result const& result) override;

    int32_t processPartial(
        int32_t const& status,
        v3::NativeDev::Result const& result,
        std::vector<v3::NativeDev::BufHandle*> const& completedBuf) override;

   public:
    SampleSession* mParent;
  };

 protected:
  std::shared_ptr<NativeCallback> mNativeCallback;
  mutable android::Mutex mExtCameraDeviceSessionLock;
  ::android::sp<IExtCameraDeviceSession> mExtCameraDeviceSession;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~SampleSession();

  static auto makeInstance(CreationParams const& rParams)
      -> std::shared_ptr<IFeatureSession>;

  explicit SampleSession(CreationParams const& rParams);

  // auto        setCallbacks(const
  // ::android::sp<::android::hardware::camera::device::V3_5::ICameraDeviceCallback>&
  // callback) -> void;

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

 public:
  // Return<Status> flush();
  // Return<void> close();

  // Return<void> configureStreams(
  //     const ::android::hardware::camera::device::V3_5::StreamConfiguration&
  //         requestedConfiguration,
  //     android::hardware::camera::device::V3_6::ICameraDeviceSession::configureStreams_3_6_cb
  //     _hidl_cb);

  // Return<void> processCaptureRequest(
  //     const
  //     hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
  //     requests, const hidl_vec<BufferCache>& cachesToRemove, const
  //     std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
  //     android::hardware::camera::device::V3_4::ICameraDeviceSession::processCaptureRequest_3_4_cb
  //     _hidl_cb);

  Return<Status> flush() override;
  Return<void> close() override;

  Return<void> configureStreams(
      const ::android::hardware::camera::device::V3_5::StreamConfiguration&
          requestedConfiguration,
      android::hardware::camera::device::V3_6::ICameraDeviceSession::
          configureStreams_3_6_cb _hidl_cb) override;

  Return<void> processCaptureRequest(
      const hidl_vec<::android::hardware::camera::device::V3_4::CaptureRequest>&
          requests,
      const hidl_vec<BufferCache>& cachesToRemove,
      const std::vector<mcam::hidl::RequestBufferCache>& vBufferCache,
      android::hardware::camera::device::V3_4::ICameraDeviceSession::
          processCaptureRequest_3_4_cb _hidl_cb) override;

  auto setMetadataQueue(
      std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
      std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue)
      -> void override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  Return<void> notify(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_2::NotifyMsg>& msgs) override;

  Return<void> processCaptureResult(
      const ::android::hardware::hidl_vec<
          ::android::hardware::camera::device::V3_4::CaptureResult>& results)
      override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  auto setExtCameraDeviceSession(
      const ::android::sp<IExtCameraDeviceSession>& session)
      -> ::android::status_t;

 protected:
  void processOneCaptureResult(
      const ::android::hardware::camera::device::V3_4::CaptureResult& result);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  ::android::sp<::android::hardware::camera::device::V3_6::ICameraDeviceSession>
      mSession = nullptr;
  ::android::sp<
      ::android::hardware::camera::device::V3_5::ICameraDeviceCallback>
      mCameraDeviceCallback = nullptr;
  int32_t mInstanceId = -1;
  const camera_metadata* mCameraCharacteristics;
  std::vector<int32_t> mvSensorId;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int mAtMostMetaStreamCount = 0;
  camera_metadata* append_Result = nullptr;
  int32_t mStreamConfigCounter = -1;

 protected:  ////                Metadata Fast Message Queue (FMQ)
  std::shared_ptr<RequestMetadataQueue> mRequestMetadataQueue;
  std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

 private:
  ::android::hardware::camera::device::V3_4::Stream mTuningStream;
  ::android::hardware::camera::device::V3_4::Stream mPreviewStream;
  ::android::hardware::camera::device::V3_4::Stream mPreviewWorkingStream;
  camera_metadata* mLastSettings = nullptr;

  uint64_t mTuningStreamBufferSerialNo = 1;
  uint64_t mPreviewWorkingStreamBufferSerialNo = 1;

  AllocatorHelper& mAllocator = AllocatorHelper::getInstance();
  MapperHelper& mMapper = MapperHelper::getInstance();

 private:
  // Native ISP
  v3::NativeDev::NativeModule mNativeModule;
  std::shared_ptr<v3::NativeDev::INativeDev> mNativeDevice;
  v3::NativeDev::Stream mTuningStream_native;
  v3::NativeDev::Stream mPreviewInStream_native;
  v3::NativeDev::Stream mPreviewOutStream_native;

  // inflight request store information of every requests.
  struct InflightRequest {
    // this structure keep images/metadata from MTK Zone HAL3+ callback
    // when all images/metadata are ready, implementor must construct Native ISP
    // request
    struct PostProc {
      camera_metadata* pAppCtrl = nullptr;
      camera_metadata* pAppResult = nullptr;
      // determine when request start
      int32_t numInputBuffers = -1;
      int32_t numOutputBuffers = -1;
      int32_t numTuningBuffers = -1;
      int32_t numManualTunings = -1;
      // check it's ready for post-processing or not
      int32_t readyInputBuffers = 0;
      int32_t readyOutputBuffers = 0;
      int32_t readyTuningBuffers = 0;
      int32_t readyManualTunings = 0;
    };
    uint32_t frameNumber;
    PostProc mPostProc;
    // keep buffer handle and id information
    const native_handle_t* mFwkPreviewBufferHd;
    uint64_t mFwkPreviewBufferId;

    // allocated by customization zone. must be freed when remove request
    ::android::hardware::hidl_handle mPreviewBufferHd;
    ::android::hardware::hidl_handle mTuningBufferHd;

    auto dump() -> void;

    auto isReadyForPostProc() -> bool {
      return mPostProc.pAppCtrl && mPostProc.pAppResult &&
             mPostProc.numInputBuffers == mPostProc.readyInputBuffers &&
             mPostProc.numOutputBuffers == mPostProc.readyOutputBuffers &&
             mPostProc.numTuningBuffers == mPostProc.readyTuningBuffers &&
             mPostProc.numManualTunings == mPostProc.readyManualTunings;
    }
    bool bRemovable = false;
  };
  mutable std::mutex mInflightMapLock;
  std::map<uint32_t, InflightRequest> mInflightRequests;

  // cached by CusCameraDeviceSession
  // std::map<uint64_t, native_handle_t*>      mBufferCacheMap;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace sample
};      // namespace feature
};      // namespace custom_dev3
};      // namespace NSCam
#endif  // MAIN_MTKHAL_CUSTOM_HIDL_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_
