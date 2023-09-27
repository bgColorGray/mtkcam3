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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef MAIN_HAL_CUSTOM_DEVICE_3_X_FEATURE_MULTICAM_MULTICAMSESSION_H_
#define MAIN_HAL_CUSTOM_DEVICE_3_X_FEATURE_MULTICAM_MULTICAMSESSION_H_

#include <mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h>

#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <main/mtkhal/core/utils/imgbuf/include/IIonImageBufferHeap.h>
#include <main/mtkhal/core/device/3.x/include/ICallbackCore.h>

#include <set>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../IFeatureSession.h"

template<typename Value_T>
using SensorMap = std::map<uint32_t, Value_T>;

/******************************************************************************
 * A sample module derived from IFeatureSession for bypass mode.
 ******************************************************************************/
namespace mcam {
namespace custom {
namespace feature {
namespace multicam {

/******************************************************************************
 *
 ******************************************************************************/
class MulticamSession : public IFeatureSession {
 public:
  struct PostProcListener : public mcam::IMtkcamDeviceCallback {
   public:
    MulticamSession* mParent;

    explicit PostProcListener(MulticamSession* pParent);

    static auto create(MulticamSession* pParent) -> PostProcListener*;

    virtual auto processCaptureResult(
                  const std::vector<mcam::CaptureResult>& results) -> void;

    virtual auto notify(const std::vector<mcam::NotifyMsg>& msgs) -> void;
  };

 protected:
  std::shared_ptr<PostProcListener> mPostProcListener;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Operations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////        Instantiation.
  virtual ~MulticamSession();

  static auto makeInstance(CreationParams const& rParams)
      -> std::shared_ptr<IFeatureSession>;

  explicit MulticamSession(CreationParams const& rParams);

 private:  ////        Operations.
  auto const& getLogPrefix() const { return mLogPrefix; }

 public:
  int configureStreams(
        const mcam::StreamConfiguration& requestedConfiguration,
        mcam::HalStreamConfiguration& halConfiguration) override;
  int processCaptureRequest(
        const std::vector<mcam::CaptureRequest>& requests,
        uint32_t& numRequestProcessed) override;
  int flush() override;
  int close() override;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:
  void notify(const std::vector<mcam::NotifyMsg>& msgs) override;
  void processCaptureResult(
        const std::vector<mcam::CaptureResult>& results) override;

 protected:
  void processOneCaptureResult(const mcam::CaptureResult& result);
  // bool processMasterSelection(const mcam::CaptureResult& result);
  bool processMasterSelection(
        const mcam::CaptureResult& result,
        mcam::StreamBuffer& selectedBuffer);
  void postProcProcessCompleted(const std::vector<CaptureResult>& results);

 private:
  bool getActiveRegionBySensorId(
        int32_t const sensorId,
        NSCam::MRect& activeArray);

  bool dumpStreamBuffer(
        const StreamBuffer& streamBuf,
        std::string prefix_msg,
        uint32_t frameNumber);

 protected:
  int32_t mInstanceId = -1;
  std::vector<int32_t> mvSensorId;
  const NSCam::IMetadata mCameraCharacteristics;
  NSCam::IMetadata append_Result;
  //
  std::shared_ptr<mcam::IMtkcamDeviceSession> mSession = nullptr;
  std::shared_ptr<mcam::custom::CusCameraDeviceCallback>
      mCameraDeviceCallback = nullptr;

 protected:
  std::mutex mInterfaceLock;
  std::string const mLogPrefix;

 private:
  NSCam::IMetadata mLastSettings;
  uint64_t mWorkingStreamBufferSerialNo = 1;

  // post proc part
  std::shared_ptr<mcam::IMtkcamProvider> mPostProcProvider;
  std::shared_ptr<mcam::IMtkcamDevice> mPostProcDevice;
  std::shared_ptr<mcam::IMtkcamDeviceSession> mPostProcDeviceSession;

 private:
  // real preview stream from app
  mcam::Stream mPreviewStream;
  // working preview stream to data collection
  // mcam::Stream mPreviewWorkingStream;

  // request time: selected physical stream for post-proc
  mcam::Stream mSelectedMasterStream;

  // ISP7: physical YUV from P1 (6s from P2)
  // mcam::Stream mPhysicalOutStream_0;
  // mcam::Stream mPhysicalOutStream_1;

  // for multiple physical streams (P1: R1, R2, ME, .., or P2)
  SensorMap<std::vector<mcam::Stream>> mvCorePhysicalOutputStreams;

  mcam::Stream mPostProcStreamIn;
  SensorMap<std::vector<mcam::Stream>> mvPostProcInputStreams;
  mcam::Stream mPostProcStreamOut;

  // target physical sensor for data collection
  std::set<int32_t> mPhysicalSensorSet = {0, 2};
  const std::vector<int32_t> mvPhysicalSensorList = {0, 2};

  // device(0,1) => use 0,1; device(0,2,3) => use 0,2
  uint32_t cam_first = mvPhysicalSensorList[0];
  uint32_t cam_second = mvPhysicalSensorList[1];

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
      // check it's ready for post-processing or not
      int32_t readyInputBuffers = 0;
      int32_t readyOutputBuffers = 0;
    };

    PostProc mPostProc;
    uint32_t frameNumber;
    float mZoomRatio = 0.0;
    bool isErrorResult = false;

    // keep buffer handle and id information
    std::shared_ptr<mcam::IImageBufferHeap> mFwkPreviewBufferHd;
    uint64_t mFwkPreviewBufferId;

    // custom zone physical working buffer
    std::shared_ptr<mcam::IImageBufferHeap> mPreviewBufferHd;
    std::shared_ptr<mcam::IImageBufferHeap> mPhysicalBufferHandle_0;
    std::shared_ptr<mcam::IImageBufferHeap> mPhysicalBufferHandle_1;
    uint64_t mPreviewBufferId = 0;
    uint64_t mPhysicalBufferId_0 = 0;
    uint64_t mPhysicalBufferId_1 = 0;

    auto dump() -> void;
    auto isReadyForPostProc() -> bool {
      return !(mPostProc.pAppCtrl.isEmpty()) &&
             !(mPostProc.pAppResult.isEmpty()) &&
             mPostProc.numInputBuffers == mPostProc.readyInputBuffers &&
             mPostProc.numOutputBuffers == mPostProc.readyOutputBuffers;
    }
    bool bRemovable = false;
  };
  mutable std::mutex mInflightMapLock;
  std::map<uint32_t, InflightRequest> mInflightRequests;
};

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace multicam
};      // namespace feature
};      // namespace custom
};      // namespace mcam
#endif  // MAIN_HAL_CUSTOM_DEVICE_3_X_FEATURE_MULTICAM_MULTICAMSESSION_H_
