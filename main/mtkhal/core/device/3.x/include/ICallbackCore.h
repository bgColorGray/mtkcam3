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

#ifndef MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKCORE_H_
#define MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKCORE_H_
//
// #include "IAppCommonStruct.h"
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>

#include <mtkcam/utils/debug/IPrinter.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>
#include <utils/Vector.h>
#include <inttypes.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include "mtkcam3/pipeline/stream/IStreamBuffer.h"
#include "mtkcam3/pipeline/stream/IStreamBufferProvider.h"
#include "mtkcam3/pipeline/stream/IStreamInfo.h"
#include "mtkcam3/pipeline/stream/IStreamInfoBuilder.h"
//#include "mtkcam-interfaces/pipeline/utils/stream/Factory.h"
#pragma GCC diagnostic pop

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {

/**
 * An Interface for control Callback Module specific behavior
 * Partial option enum works on CallbackCore and the others
 * works on Adaptor.
 */
enum EnableOption {
  eCORE_NONE = 0x00000000U,
  //
  eCORE_MTK_METAMERGE = 0x00000001U,
  eCORE_MTK_METAPENDING = 0x00000002U,
  eCORE_MTK_RULE_MASK = 0x0000000FU,
  //
  eCORE_AOSP_META_RULE = 0x00000010U,
  eCORE_AOSP_RULE_MASK = 0x000000F0U,
  //
  eCORE_BYPASS = 0x10000000U,
};
class CoreSetting {
 public:
  explicit CoreSetting(int64_t setting);
  //
 public:
  inline bool enableMetaMerge() {
    return (mSetting & eCORE_MTK_METAMERGE) == eCORE_MTK_METAMERGE;
  }
  inline bool enableMetaPending() {
    return (mSetting & eCORE_MTK_METAPENDING) == eCORE_MTK_METAPENDING;
  }
  inline bool enableAOSPMetaRule() {
    return (mSetting & eCORE_AOSP_META_RULE) == eCORE_AOSP_META_RULE;
  }
  inline bool enableCallbackCore() {
    return (mSetting & eCORE_BYPASS) == 0;
  }
 private:
  void validateSetting();

  ::android::String8 toString();

 private:
  int64_t mSetting = 0;
};

/**
 * An interface of App stream manager.
 */
class ICallbackCore : public virtual ::android::RefBase {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  struct CallbackParcel;
  typedef void (*PFN_CALLBACK_T)(std::list<CallbackParcel>&, void*);

  struct CreationInfo {
    int32_t mInstanceId;
    int32_t mLogLevel;
    bool mSupportBufferManagement = false;
    size_t mAtMostMetaStreamCount = 0;
    //
    PFN_CALLBACK_T mpfnCallback;
    void* mpUser;
    std::string userName;
    std::shared_ptr<CoreSetting> coreSetting = nullptr;
  };

  struct StreamInfo : public mcam::Stream {
    BufferUsage producerUsage = 0;
    BufferUsage consumerUsage = 0;
    const int64_t getStreamId() { return id; }
  };

 public:  ////    Input structure
  struct ShutterResult : public ShutterMsg {
    int64_t startOfFrame = 0;
  };

  struct VecMetaResult : public std::vector<IMetadata> {
    bool isRealTimePartial = false;
    bool hasLastPartial = false;
  };

  struct VecPhysicMetaResult
      : public std::vector<std::shared_ptr<PhysicalCameraMetadata>> {};

  struct VecImageResult : public std::vector<std::shared_ptr<StreamBuffer>> {};

  struct ResultQueue {
    std::map<uint32_t, std::shared_ptr<ShutterResult>> vShutterResult;
    std::map<uint32_t, std::shared_ptr<ErrorMsg>> vMessageResult;
    std::map<uint32_t, VecMetaResult> vMetaResult;
    std::map<uint32_t, VecMetaResult> vHalMetaResult;
    std::map<uint32_t, VecPhysicMetaResult> vPhysicMetaResult;
    std::map<uint32_t, VecImageResult> vOutputImageResult;
    std::map<uint32_t, VecImageResult> vInputImageResult;
    bool empty() {
      return vMetaResult.empty() && vHalMetaResult.empty() &&
             vPhysicMetaResult.empty() && vOutputImageResult.empty() &&
             vInputImageResult.empty() && vMessageResult.empty() &&
             vShutterResult.empty();
    }
    void clear() {
      vMetaResult.clear();
      vHalMetaResult.clear();
      vPhysicMetaResult.clear();
      vOutputImageResult.clear();
      vInputImageResult.clear();
      vMessageResult.clear();
      vShutterResult.clear();
    }
  };

 public:  ////    Output structure
  struct CallbackParcel {
    struct ImageItem {
      std::shared_ptr<StreamBuffer> buffer;
      std::shared_ptr<StreamInfo> stream;
    };

    struct MetaItem {
      IMetadata metadata;
      uint32_t bufferNo = 0;  // partial_result
    };

    struct Error {
      std::shared_ptr<StreamInfo> stream;
      ErrorCode errorCode = ErrorCode::ERROR_REQUEST;
    };

    ::android::Vector<ImageItem> vInputImageItem;
    ::android::Vector<ImageItem> vOutputImageItem;
    ::android::Vector<MetaItem> vOutputMetaItem;
    ::android::Vector<MetaItem> vOutputHalMetaItem;
    ::android::Vector<PhysicalCameraMetadata> vOutputPhysicMetaItem;
    ::android::Vector<Error> vError;
    std::shared_ptr<ShutterResult> shutter;
    uint32_t frameNumber = 0;
    bool valid = false;
    bool needIgnore = false;
    bool needRemove = false;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////    Operations.
  static auto create(const CreationInfo& creationInfo)
      -> std::shared_ptr<ICallbackCore>;

  /**
   * Destroy the instance.
   */
  virtual auto destroy() -> void = 0;

  /**
   * Dump debugging state.
   */
  virtual auto dumpState(NSCam::IPrinter& printer,
                         const std::vector<std::string>& options) -> void = 0;

  /**
   * @brief beginConfiguration
   *
   * @param rStreams : the stream configuration requested by upper layer
   *
   * @return
   *    0 indicates success; otherwise failure.
   */
  virtual auto beginConfiguration(StreamConfiguration const& rStreams)
      -> void = 0;

  /**
   * @brief endConfiguration
   *
   * @param rHalStreams : the stream configuration result generated by HAL
   *
   * @return
   *    0 indicates success; otherwise failure.
   */
  virtual auto endConfiguration(HalStreamConfiguration const& rHalStreams)
      -> void = 0;

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
  // virtual auto    getCaptureRequestMetadataQueue() -> const
  // ::::android::hardware::MQDescriptorSync<uint8_t>&
  //                                                                         =
  //                                                                         0;

  /**
   * Flush requests.
   *
   * @param[in] requests: the requests to flush.
   *
   */
  virtual auto flushRequest(const std::vector<CaptureRequest>& requests)
      -> void = 0;

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
  virtual auto submitRequests(const std::vector<CaptureRequest>& rRequests)
      -> int = 0;

  /**
   * Wait until all the registered requests have finished returning.
   *
   * @param[in] timeout
   */
  virtual auto waitUntilDrained(nsecs_t const timeout) -> int = 0;

  /**
   * abort reuqests failed to submit to pipeline
   *
   * @param[in] requests: a set of given requests in terms of a form of
   *  CaptureRequest
   *
   */
  virtual auto abortRequest(const std::vector<uint32_t> frameNumbers)
      -> void = 0;

  virtual auto updateResult(ResultQueue const& param) -> void = 0;
};

static inline void convertToDebugString(
    ICallbackCore::CallbackParcel const& cbParcel,
    ::android::Vector<::android::String8>& out) {
  /*
        frameNumber 80 errors#:10 shutter:10 o:meta#:10 i:image#:10 o:image#:10
            {ERROR_DEVICE/ERROR_REQUEST/ERROR_RESULT}
            {ERROR_BUFFER streamId:123 bufferId:456}
            OUT Meta  -
                partial#:123 "xxxxxxxxxx"
            IN Image -
                streamId:01 bufferId:04 OK
            OUT Image -
                streamId:02 bufferId:04 ERROR with releaseFence
    */
  ::android::String8 log =
      ::android::String8::format("frameNumber %u", cbParcel.frameNumber);
  if (!cbParcel.vError.empty()) {
    log += ::android::String8::format(" errors#:%zu", cbParcel.vError.size());
  }
  if (cbParcel.shutter != 0) {
    log += ::android::String8::format(" shutter:%" PRId64 "",
                                      cbParcel.shutter->timestamp);
  }
  if (!cbParcel.vOutputMetaItem.empty()) {
    log += ::android::String8::format(" o:meta#:%zu",
                                      cbParcel.vOutputMetaItem.size());
  }
  if (!cbParcel.vOutputHalMetaItem.empty()) {
    log += ::android::String8::format(" o:hal meta#:%zu",
                                      cbParcel.vOutputHalMetaItem.size());
  }
  if (!cbParcel.vOutputPhysicMetaItem.empty()) {
    log += ::android::String8::format(" o:physic meta#:%zu",
                                      cbParcel.vOutputPhysicMetaItem.size());
  }
  if (!cbParcel.vInputImageItem.empty()) {
    log += ::android::String8::format(" i:image#:%zu",
                                      cbParcel.vInputImageItem.size());
  }
  if (!cbParcel.vOutputImageItem.empty()) {
    log += ::android::String8::format(" o:image#:%zu",
                                      cbParcel.vOutputImageItem.size());
  }
  out.push_back(log);
  //
  for (auto const& item : cbParcel.vError) {
    out.push_back(::android::String8::format("    {%s}",
                                             toString(item.errorCode).c_str()));
  }
  //
  if (!cbParcel.vOutputMetaItem.empty()) {
    out.push_back(::android::String8("    OUT Meta  -"));
    for (auto const& item : cbParcel.vOutputMetaItem) {
      out.push_back(::android::String8::format("        partial#:%u count:%u",
                                               item.bufferNo,
                                               item.metadata.count()));
    }
  }
  //
  if (!cbParcel.vOutputHalMetaItem.empty()) {
    out.push_back(::android::String8("    OUT Hal Meta  -"));
    for (auto const& item : cbParcel.vOutputHalMetaItem) {
      out.push_back(::android::String8::format("        partial#:%u count:%u",
                                               item.bufferNo,
                                               item.metadata.count()));
    }
  }
  //
  if (!cbParcel.vOutputPhysicMetaItem.empty()) {
    out.push_back(::android::String8("    OUT Physical Meta  -"));
    for (auto const& item : cbParcel.vOutputPhysicMetaItem) {
      out.push_back(::android::String8::format(
          "       physical CamId#:%d count:%u", item.physicalCameraId,
          item.metadata.count()));
    }
  }
  //
  if (!cbParcel.vInputImageItem.empty()) {
    out.push_back(::android::String8("     IN Image -"));
    for (auto const& item : cbParcel.vInputImageItem) {
      out.push_back(::android::String8::format(
          "        streamId:%02" PRId32 " bufferId:%02" PRIu64 " %s %s",
          item.buffer->streamId, item.buffer->bufferId,
          (item.buffer->status == BufferStatus::ERROR ? "ERROR" : "OK"),
          (item.buffer->releaseFenceFd == -1 ? "" : "with releaseFence")));
    }
  }
  //
  if (!cbParcel.vOutputImageItem.empty()) {
    out.push_back(::android::String8("    OUT Image -"));
    for (auto const& item : cbParcel.vOutputImageItem) {
      if (item.buffer) {
        out.push_back(::android::String8::format(
            "        streamId:%02" PRId32 " bufferId:%02" PRIu64 " %s %s",
            item.buffer->streamId, item.buffer->bufferId,
            (item.buffer->status == BufferStatus::ERROR ? "ERROR" : "OK"),
            (item.buffer->releaseFenceFd == -1 ? "" : "with releaseFence")));
      } else {
        out.push_back(::android::String8::format("        streamId:%02" PRId32
                                                 " buffer is nullptr",
                                                 item.stream->id));
      }
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
};      // namespace core
};      // namespace mcam
#endif  // MAIN_MTKHAL_CORE_DEVICE_3_X_INCLUDE_ICALLBACKCORE_H_
