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

#ifndef MAIN_MTKHAL_CUSTOM_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_
#define MAIN_MTKHAL_CUSTOM_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_

// #include "../../graphics/AllocatorHelper.h"
// #include "../../graphics/MapperHelper.h"

// #include <mtkcam-android/main/standalone_isp/NativeDev/INativeCallback.h>
// #include <mtkcam-android/main/standalone_isp/NativeDev/INativeDev.h>
// #include <mtkcam-android/main/standalone_isp/NativeDev/NativeModule.h>
//#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
#include <main/mtkhal/core/utils/imgbuf/include/IIonImageBufferHeap.h>

#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h>

// #include <utils/Mutex.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../IFeatureSession.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace custom {
namespace feature {
namespace sample {

/******************************************************************************
 * A sample module derived from IFeatureSession to implement simple example for
 * MTK HAL3+ I/F to integrate data collection and isp reprocess flow.
 ******************************************************************************/
class SampleSession : public IFeatureSession {
  // friend class NativeCallback;

 public:
  // struct NativeCallback : public NSCam::v3::NativeDev::INativeCallback {
  //   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //   //  INativeCallback
  //   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  //  public:
  //   int32_t processCompleted(
  //       int32_t const& status,
  //       NSCam::v3::NativeDev::Result const& result) override;

  //   int32_t processPartial(int32_t const& status,
  //                          NSCam::v3::NativeDev::Result const& result,
  //                          std::vector<NSCam::v3::NativeDev::BufHandle*>
  //                          const&
  //                              completedBuf) override;

  //  public:
  //   SampleSession* mParent;
  // };

  struct PostProcListener : public mcam::IMtkcamDeviceCallback {
   public:
    SampleSession* mParent;

    explicit PostProcListener(SampleSession* pParent);

    static auto create(SampleSession* pParent) -> PostProcListener*;

    virtual auto processCaptureResult(
        const std::vector<mcam::CaptureResult>& results) -> void;

    virtual auto notify(const std::vector<mcam::NotifyMsg>& msgs) -> void;
  };

 protected:
  // std::shared_ptr<NativeCallback> mNativeCallback;
  std::shared_ptr<PostProcListener> mPostProcListener;
  // mutable std::mutex mExtCameraDeviceSessionLock;
  // std::shared_ptr<IExtCameraDeviceSession> mExtCameraDeviceSession;

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
  /*
  auto getPostProcProviderInstance(IProviderCreationParams const* params)
    -> std::shared_ptr<mcam::IMtkcamProvider>;
    */

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

  int flush() override;
  int close() override;

  int configureStreams(
      const mcam::StreamConfiguration& requestedConfiguration,
      mcam::HalStreamConfiguration& halConfiguration) override;

  int processCaptureRequest(
      const std::vector<mcam::CaptureRequest>& requests,
      uint32_t& numRequestProcessed) override;

  // auto setMetadataQueue(
  //     std::shared_ptr<RequestMetadataQueue>& pRequestMetadataQueue,
  //     std::shared_ptr<ResultMetadataQueue>& pResultMetadataQueue)
  //     -> void override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void notify(const std::vector<mcam::NotifyMsg>& msgs) override;

  void processCaptureResult(
      const std::vector<mcam::CaptureResult>& results) override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  // auto setExtCameraDeviceSession(const
  // std::shared_ptr<IExtCameraDeviceSession>& session)
  //     -> int;

  void postProcProcessCompleted(const std::vector<CaptureResult>& results);

 protected:
  void processOneCaptureResult(const mcam::CaptureResult& result);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:
  std::shared_ptr<mcam::IMtkcamDeviceSession> mSession = nullptr;
  std::shared_ptr<mcam::custom::CusCameraDeviceCallback>
      mCameraDeviceCallback = nullptr;
  int32_t mInstanceId = -1;
  const NSCam::IMetadata mCameraCharacteristics;
  std::vector<int32_t> mvSensorId;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;
  int mAtMostMetaStreamCount = 0;
  NSCam::IMetadata append_Result;
  int32_t mStreamConfigCounter = -1;

 protected:  ////                Metadata Fast Message Queue (FMQ)
             // std::shared_ptr<RequestMetadataQueue> mRequestMetadataQueue;
             // std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;
 private:
  mcam::Stream mTuningStream;
  mcam::Stream mPreviewStream;
  mcam::Stream mPreviewWorkingStream;
  IMetadata mLastSettings;

  uint64_t mTuningStreamBufferSerialNo = 1;
  uint64_t mPreviewWorkingStreamBufferSerialNo = 1;

  // NSCam::AllocatorHelper& mAllocator = NSCam::AllocatorHelper::getInstance();
  // NSCam::MapperHelper& mMapper = NSCam::MapperHelper::getInstance();

 private:
  // Native ISP
  // NSCam::v3::NativeDev::NativeModule mNativeModule;
  // std::shared_ptr<NSCam::v3::NativeDev::INativeDev> mNativeDevice;
  std::shared_ptr<mcam::IMtkcamProvider> mPostProcProvider;
  std::shared_ptr<mcam::IMtkcamDevice> mPostProcDevice;
  std::shared_ptr<mcam::IMtkcamDeviceSession> mPostProcDeviceSession;
  mcam::Stream mTuningStream_native;
  mcam::Stream mPreviewInStream_native;
  mcam::Stream mPreviewOutStream_native;

  mcam::Stream mPreviewInStream_PostProc;
  mcam::Stream mPreviewOutStream_PostProc;

  // inflight request store information of every requests.
  struct InflightRequest {
    // this structure keep images/metadata from MTK Zone HAL3+ callback
    // when all images/metadata are ready, implementor must construct Native ISP
    // request
    struct PostProc {
      NSCam::IMetadata pAppCtrl;
      NSCam::IMetadata pAppResult;
      NSCam::IMetadata pHalResult;
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
    bool isErrorResult = false;
    // keep buffer handle and id information
    std::shared_ptr<mcam::IImageBufferHeap> mFwkPreviewBufferHd;
    uint64_t mPreviewBufferId = 0;
    uint64_t mFwkPreviewBufferId;
    // allocated by customization zone. must be freed when remove request
    // ::android::hardware::hidl_handle mPreviewBufferHd;
    // ::android::hardware::hidl_handle mTuningBufferHd;

    std::shared_ptr<mcam::IImageBufferHeap> mPreviewBufferHd;
    std::shared_ptr<mcam::IImageBufferHeap> mTuningBufferHd;

    auto dump() -> void;

    auto isReadyForPostProc() -> bool {
      return !(mPostProc.pAppCtrl.isEmpty()) &&
             !(mPostProc.pAppResult.isEmpty()) &&
             mPostProc.numInputBuffers == mPostProc.readyInputBuffers &&
             mPostProc.numOutputBuffers == mPostProc.readyOutputBuffers &&
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
};      // namespace custom
};      // namespace mcam
#endif  //  MAIN_MTKHAL_CUSTOM_DEVICE_3_X_FEATURE_SAMPLE_SAMPLESESSION_H_
