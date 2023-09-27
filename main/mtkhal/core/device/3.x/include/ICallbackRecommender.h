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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKRECOMMENDER_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKRECOMMENDER_H_

#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
#include <functional>
#include <memory>
#include <vector>

/* ICallbackRecommender implements AOSP callback rules defined in
 * ICameraDeviceCallback or camera3_callback_ops_t.
 *
 * - shutter
 * - error
 * - image buffers
 * - final result metadata
 * - physical result metadata
 */

namespace mcam {
namespace core {
namespace Utils {

class ICallbackRecommender {
 public:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // instaintiate
  struct CreationInfo {
    uint32_t partialResultCount;
  };

  // // configuration
  // class StreamInfo {
  //  public:
  //   int64_t streamId = -1;
  //   // physicalCameraId: user must set this value if stream comes from
  //   physical
  //   // device
  //   int64_t physicalCameraId = -1;
  // };

  // class ConfiguredStreamInfos {
  //  public:
  //   std::vector<StreamInfo> vInputStreams;
  //   std::vector<StreamInfo> vOutputStreams;
  // };

  // request
  enum RequestType {
    eREQUEST_NORMAL = 0,
    eREQUEST_ZSL_STILL_CAPTURE = 1,
    eREQUEST_REPROCESSING = 2,
  };

  class CaptureRequest {
   public:
    uint32_t frameNumber = 0;
    std::vector<int64_t> vInputStreamIds;
    std::vector<int64_t> vOutputStreamIds;
    RequestType requestType = eREQUEST_NORMAL;
  };

  // callback
  // enum BufferStatus {
  //   eBUFFER_OK = 0,
  //   eBUFFER_ERROR = 1,
  // };

  class Stream {
   public:
    int64_t streamId = -1;
    // physicalCameraId: User must set this value if stream is came from
    // physical device
    BufferStatus status = BufferStatus::OK;
  };

  class CaptureResult {
   public:
    uint32_t frameNumber = 0;
    std::vector<Stream> vInputStreams;
    std::vector<Stream> vOutputStreams;
    // hasLastPartial: MUST be true if and only if recieving all logical and
    // physical results
    bool hasLastPartial = false;
    std::vector<int64_t> physicalCameraIdsOfPhysicalResults;
  };

  // enum ErrorType {
  //   eERROR_NONE = 0,
  //   eERROR_DEVICE = 1,
  //   eERROR_REQUEST = 2,
  //   eERROR_RESULT = 3,
  //   eERROR_BUFFER = 4,
  // };

  // class ErrorMsg {
  //  public:
  //   uint32_t frameNumber = 0;
  //   // errorStreamId: only valid if errorType==eERROR_BUFFER
  //   int64_t errorStreamId = -1;
  //   ErrorType errorType = eERROR_DEVICE;
  // };

  /*
   * Return callback for shutter/error/image/result which are available for
   * callback.
   *
   */
  using availableCallback_cb = std::function<void(
      /* for notify */
      const std::vector<uint32_t>& availableShutters,
      const std::vector<ErrorMsg>& availableErrors,
      /* for processCaptureResult */
      const std::vector<CaptureResult>& availableBuffers,
      const std::vector<CaptureResult>& availableResults,
      /* for completed requests */
      const std::vector<uint32_t>& removableRequests)>;

 public:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  static auto createInstance(CreationInfo& creationInfo)
      -> std::shared_ptr<ICallbackRecommender>;

  // definition
  virtual ~ICallbackRecommender() {}

  // configuration
  virtual auto configureStreams(const StreamConfiguration& streamInfos)
      -> int = 0;

  // request
  virtual auto registerRequests(const std::vector<CaptureRequest>& requests)
      -> int = 0;

  virtual auto removeRequests(const std::vector<uint32_t>& requestNumbers,
                              availableCallback_cb _available_cb_) -> int = 0;

  // notify
  virtual auto notifyErrors(
      const std::vector<ErrorMsg>& errorMessages,  // input
      availableCallback_cb _available_cb_)         // output
      -> void = 0;

  virtual auto notifyShutters(
      const std::vector<uint32_t>& requestNumbers,  // input
      availableCallback_cb _available_cb_)          // output
      -> void = 0;

  // processCaptureResult
  virtual auto receiveMetadataResults(
      const std::vector<CaptureResult>& results,  // input
      availableCallback_cb _available_cb_)        // output
      -> void = 0;

  virtual auto receiveBuffers(
      const std::vector<CaptureResult>& buffers,  // input
      availableCallback_cb _available_cb_)        // output
      -> void = 0;
};

};  // namespace Utils
};  // namespace core
};  // namespace mcam

#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKRECOMMENDER_H_
