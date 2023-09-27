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
#ifndef MAIN_MTKHAL_HIDL_PROVIDER_2_X_TEST_TEST_H_
#define MAIN_MTKHAL_HIDL_PROVIDER_2_X_TEST_TEST_H_

#include <inttypes.h>
#include <semaphore.h>
#include <utils/Timers.h>
#include <utils/Vector.h>
#include <utils/Thread.h>
#include <utils/KeyedVector.h>
#include <fmq/MessageQueue.h>

#include <hardware/gralloc.h>
#include <ui/GraphicBuffer.h>
#include <hardware/gralloc1.h>
#include <ui/GraphicBufferMapper.h>
#include <system/camera_metadata.h>
#include <ui/GraphicBufferAllocator.h>

#include <android/hardware/graphics/common/1.2/types.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <android/hardware/graphics/allocator/4.0/IAllocator.h>

#include <android/hardware/camera/device/1.0/ICameraDevice.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.5/ICameraDevice.h>
#include <android/hardware/camera/device/3.6/ICameraDevice.h>

#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <android/hardware/camera/provider/2.5/ICameraProvider.h>
#include <android/hardware/camera/provider/2.6/ICameraProvider.h>

#include <android/hardware/camera/device/3.3/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.6/ICameraDeviceSession.h>


#include <android/hardware/camera/device/3.4/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceCallback.h>
#include <android/hardware/camera/provider/2.6/ICameraProviderCallback.h>


#include <mtkcam-android/main/standalone_isp/NativeDev/types.h>
#include <mtkcam-android/main/standalone_isp/NativeDev/INativeDev.h>
#include <mtkcam-android/main/standalone_isp/NativeDev/NativeModule.h>
#include <mtkcam-android/main/standalone_isp/NativeDev/INativeCallback.h>
#include <../main/mtkhal/custom/hidl/device/3.x/graphics/MapperHelper.h>

#include<mtkcam3/main/mtkhal3plus/extension/device/IExtCameraDeviceSession.h>
#include <map>
#include <list>
#include <regex>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>

using ::android::hardware::Void;
using ::android::hardware::Return;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_handle;
using ::android::hardware::MessageQueue;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::common::V1_0::VendorTagSection;


using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::StreamBuffer;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_5::CameraBlob;
using ::android::hardware::camera::device::V3_5::CameraBlobId;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using android_Stream =
  ::android::hardware::camera::device::V3_4::Stream;

using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_4::PhysicalCameraMetadata;
using ::android::hardware::camera::device::V3_2::CaptureResult;

using android::hardware::graphics::allocator::V4_0::IAllocator;
using android::hardware::graphics::mapper::V4_0::IMapper;
using android::hardware::graphics::common::V1_0::PixelFormat;
using ::android::hardware::graphics::mapper::V4_0::BufferDescriptor;
using android::hardware::graphics::common::V1_2::BufferUsage;

using ResultMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;

using ::android::hardware::camera::device::V3_2::CaptureRequest;
using  ::android::hardware::camera::device::V3_5::BufferRequestStatus;
using  ::android::hardware::camera::device::V3_5::StreamBufferRet;
using  ::android::hardware::camera::device::V3_5::StreamBufferRequestError;
using  ::android::hardware::camera::device::V3_5::BufferRequest;

using ::NSCam::v3::NativeDev::Stream;
using ::NSCam::v3::NativeDev::StreamConfiguration;
using ::NSCam::v3::NativeDev::NativeBuffer;
using ::NSCam::v3::NativeDev::BufHandle;
using ::NSCam::v3::NativeDev::Buffer;
using ::NSCam::v3::NativeDev::InputBufferSet;
using ::NSCam::v3::NativeDev::InputRequest;
using ::NSCam::v3::NativeDev::InputRequestSet;
using ::NSCam::v3::NativeDev::OutputBuffer;
using ::NSCam::v3::NativeDev::OutputBufferSet;
using ::NSCam::v3::NativeDev::ProcRequest;
using ::NSCam::v3::NativeDev::Result;
using ::NSCam::v3::NativeDev::INativeDev;
using ::NSCam::v3::NativeDev::INativeCallback;
using ::NSCam::v3::NativeDev::GRAPHIC_HANDLE;
using ::NSCam::v3::NativeDev::NativeModule;

/*enum testScenario{
  TEST_ANDROID_HIDL,
  TEST_MTK_EXT_INTERFACE,
  TEST_WHOLE_HAL3_PLUS_INTERFACE,
};*/
enum testScenario {
  BASIC_YUV_STREAM,  // 1 yuv
  BASIC_RAW_STREAM,  // 1 raw
  BASIC_YUV_VARIO_ROI,  // diff ROI
  BASIC_YUV_VARIO_DMA_PORT,  // diff DMA
  MULTICAM_PHY_YUV,  // 2 yuv stream
  MULTICAM_SWITCH_SENSOR,  // 2 yuv stream with one error
  MCNR_F0,  // f0 + isp tuning
  MCNR_F0_F1,  // f0 + f1 + isp tuning
  MCNR_F0_F1_ME,  // f0 + f1 + ME + isp tuning
  MCNR_F0_F1_ME_WPE,  // f0 + f1 + ME + isp tuning + WPE map
  WPE_PQ,  // WPE + pq hint
  FD_YUV_STREAM,    // 1 yuv
  MSNR_RAW_YUV,
};


struct InFlightRequest {
  // Set by notify() SHUTTER call.
  nsecs_t shutterTimestamp;

  bool errorCodeValid;
  ErrorCode errorCode;

  // Is partial result supported
  bool usePartialResult;

  // Partial result count expected
  uint32_t numPartialResults;

  // Message queue
  std::shared_ptr<ResultMetadataQueue> resultQueue;

  // Set by process_capture_result call with valid metadata
  bool haveResultMetadata;

  // Decremented by calls to process_capture_result with valid output
  // and input buffers
  ssize_t numBuffersLeft;

  // A 64bit integer to index the frame number associated with this result.
  int64_t frameNumber;

  // The partial result count (index) for this capture result.
  int32_t partialResultCount;

  // For buffer drop errors, the stream ID for the stream that lost a buffer.
  // For physical sub-camera result errors, the Id of the physical stream
  // for the physical sub-camera.
  // Otherwise -1.
  int32_t errorStreamId;

  // If this request has any input buffer
  bool hasInputBuffer;

  // Result metadata
  CameraMetadata collectedResult;

  // Buffers are added by process_capture_result when output buffers
  // return from HAL but framework.
  ::android::Vector<StreamBuffer> resultOutputBuffers;

  std::unordered_set<std::string> expectedPhysicalResults;


  InFlightRequest() :
    shutterTimestamp(0),
    errorCodeValid(false),
    errorCode(ErrorCode::ERROR_BUFFER),
    usePartialResult(false),
    numPartialResults(0),
    resultQueue(nullptr),
    haveResultMetadata(false),
    numBuffersLeft(0),
    frameNumber(0),
    partialResultCount(0),
    errorStreamId(-1),
    hasInputBuffer(false) {}

  InFlightRequest(ssize_t numBuffers, bool hasInput,
    bool partialResults, uint32_t partialCount,
    std::shared_ptr<ResultMetadataQueue> queue = nullptr,
    int64_t frameNum = 0) :
      shutterTimestamp(0),
      errorCodeValid(false),
      errorCode(ErrorCode::ERROR_BUFFER),
      usePartialResult(partialResults),
      numPartialResults(partialCount),
      resultQueue(queue),
      haveResultMetadata(false),
      numBuffersLeft(numBuffers),
      frameNumber(frameNum),
      partialResultCount(0),
      errorStreamId(-1),
      hasInputBuffer(hasInput) {}

  InFlightRequest(ssize_t numBuffers, bool hasInput,
    bool partialResults, uint32_t partialCount,
    const std::unordered_set<std::string>& extraPhysicalResult,
    std::shared_ptr<ResultMetadataQueue> queue = nullptr) :
    shutterTimestamp(0),
    errorCodeValid(false),
    errorCode(ErrorCode::ERROR_BUFFER),
    usePartialResult(partialResults),
    numPartialResults(partialCount),
    resultQueue(queue),
    haveResultMetadata(false),
    numBuffersLeft(numBuffers),
    frameNumber(0),
    partialResultCount(0),
    errorStreamId(-1),
    hasInputBuffer(hasInput),
    expectedPhysicalResults(extraPhysicalResult) {}
};
const size_t camera_metadata_type_size[NUM_TYPES] = {
    [TYPE_BYTE]     = sizeof(uint8_t),
    [TYPE_INT32]    = sizeof(int32_t),
    [TYPE_FLOAT]    = sizeof(float),
    [TYPE_INT64]    = sizeof(int64_t),
    [TYPE_DOUBLE]   = sizeof(double),
    [TYPE_RATIONAL] = sizeof(camera_metadata_rational_t)
};

union data{
  uint8_t u8;
  int32_t i32;
  float    f;
  int64_t i64;
  double  d;
};
class Hal3Plus: public ::android::Thread {
 public:
  class extInterface;
  class nativeInterface;
  class extCallback;
  class nativeCallback;

 public:
  Hal3Plus();
  virtual ~Hal3Plus();

 public:
  int initial(std::string options, std::vector<std::string> devices);
  int getOpenId();
  void setDevice(
    ::android::sp< ::android::hardware::camera::device::V3_2::ICameraDevice>
      cameraDevice);
  void setVendorTag(hidl_vec<VendorTagSection>  vndTag);
  virtual auto requestExit() -> void;
  virtual auto readyToRun() -> android::status_t;
  void  start();

 private:
  virtual auto threadLoop() -> bool;

 private:
  int open();
  int config();
  int flush();
  bool getStreamInfo();

 private:
  int parseOptions(std::string options);
  int parseConfigFile();

  void allocateGraphicBuffer(
        ::android::hardware::camera::device::V3_4::Stream srcStream,
        StreamBuffer* dstBuffers,
        uint64_t bufferId);
  int allocateStreamBuffers();
  int constructExtRequest();
  int constructNativeRequest();
  bool getCharacteristics();
  int queueRequest();
  void setMetadata(
  ::android::hardware::hidl_vec<uint8_t> src,
  uint32_t tag,
  uint32_t entry_capacity,
  uint32_t data_capacity,
  int32_t cnt,
  void* value);
  void dumpData(const hidl_handle& bufferHandle,
               int size,
               int requestNum,
               std::string oritation);

 private:
  struct config {
    std::vector<std::string> inputStreams;
    std::vector<std::string> outputStreams;
    std::vector<std::string> metadata;
  };
  struct request {
    std::vector<std::string> info;
  };
  struct meta {
    std::string name;
    int type;
    std::vector<data> value;
  };
  struct input {
    std::vector<data> id;
    std::vector<struct meta> metadata;
  };

  struct output {
    std::vector<data> id;
    std::vector<struct meta> metadata;
    std::string reqType;
    std::vector<struct meta> checkMeta;
  };
  struct req {
    struct input reqInput;
    struct output reqOutput;
  };

  struct onePlane {
    int planeId;
    int width;
    int height;
    int stride;
  };
  struct oneStream {
     int streamId;
     std::vector<struct onePlane> planes;
  };

  struct TestROIParam{
    bool bTestROI;
    enum testScenario runCase;
    std::vector<data> Ids;
    int openId;
  };


  std::vector<struct oneStream> allocateInfo;
  std::vector<struct req> hal3reqs;
  std::vector<struct req> ispreqs;
  std::vector<struct meta> sessionConfig;
  hidl_vec<VendorTagSection> mVendorTagSections;
  int openId;
  int intanceId;
  int runTimes;
  int P1DmaCase;
  std::string deviceType;
  enum testScenario runCase;
  bool bEnableTuningData;
  bool bEnableNativeDevice;
  bool bEnableDump;
  struct config hal3streamConfig;
  struct config ispstreamConfig;
  std::vector<struct request> hal3Requests;
  std::vector<struct request> ispRequests;
  bool supportsPartialResults;
  bool nativeNeedsAllocInput;
  uint32_t partialResultCount = 0;
  ::android::hardware::hidl_vec<uint8_t> settings;

  ::android::sp<IAllocator> mAllocator;
  ::android::sp<IMapper> mMapper;

  CameraMetadata cameraCharacteristics;
  CameraMetadata physicalCharacteristics;
  std::vector<std::string> deviceNameList;

  ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice>
                                                               pCameraDevice;

  // ext related
  android::sp<extInterface> mExtHandler = nullptr;
  android::sp<extCallback> mExtCallbackHanlder = nullptr;

  // native related
  android::sp<nativeInterface> mNativeHandler = nullptr;
  std::shared_ptr<nativeCallback> mNativeCallbackHandler = nullptr;

  ::android::hardware::hidl_vec<StreamBuffer> extOutputBuffers;
  ::android::KeyedVector<int, ::android::hardware::hidl_vec<StreamBuffer>>
                                              extOutputBufMap;
  ::android::hardware::hidl_vec<StreamBuffer> ispTuningBuffers;
  ::android::hardware::hidl_vec<StreamBuffer> nativeInputBuffers;
  ::android::hardware::hidl_vec<StreamBuffer> nativeOutputBuffers;
  std::unordered_map<int, ::android::hardware::hidl_vec<StreamBuffer>>
                                              nativeOutputBufMap;

 public:
  int maxBuffers;
  int targetNum;
  std::mutex extLock;
  std::mutex nativeLock;
  std::vector<int> receivedFrame;
  std::condition_variable extCondition;
  std::condition_variable nativeCondition;
  std::vector<CameraMetadata> appMetadata;
  ::android::hardware::hidl_vec
    <::android::hardware::camera::device::V3_4::Stream> extOutputStreams;
  ::android::hardware::hidl_vec
    <::android::hardware::camera::device::V3_4::Stream> nativeOutputStreams;
  ::android::hardware::hidl_vec
    <::android::hardware::camera::device::V3_4::Stream> nativeInputStreams;
  ::android::hardware::hidl_vec
    <::android::hardware::camera::device::V3_4::Stream> nativeAllocInputStreams;
  std::map<int, ::android::hardware::hidl_vec<StreamBuffer>>
                                              totalAvailableBufMap;
  std::map<int, ::android::hardware::hidl_vec<StreamBuffer>>
                                              extAvailableBufMap;
  std::map<int, ::android::hardware::hidl_vec<StreamBuffer>>
                                              nativeAvailableBufMap;

  typedef ::android::KeyedVector<uint32_t, struct InFlightRequest*>
                                              InFlightMap;
  InFlightMap extInflightMap;
  InFlightMap nativeInflightMap;
  std::vector<std::shared_ptr<ProcRequest>> ProcVec;

 private:
  void setMeta(::android::hardware::hidl_vec<uint8_t>& src,
                  uint32_t tag,
                  uint32_t entry_capacity,
                  uint32_t data_capacity,
                  int32_t cnt,
                  void* value);
};

class Hal3Plus::extInterface: public android::RefBase {
 public:
    explicit extInterface(android::sp<Hal3Plus> pHal3);

 public:
  void open(::android::sp
              <::android::hardware::camera::device::V3_6::ICameraDevice> pDev,
            android::sp<extCallback> cb);

  void constructDefaultSetting(
           bool enableTuningData,
           ::android::hardware::hidl_vec<uint8_t>* defaultSetting);

  void configure(::android::hardware::hidl_vec
                   <::android::hardware::camera::device::V3_4::Stream>* streams,
                 int* maxBuf,
                 bool enableTuningData,
                 ::android::hardware::hidl_vec<uint8_t>* defaultSetting);
  void processCaptureRequest(
                 ::android::hardware::hidl_vec<StreamBuffer> outputBuffer,
                 StreamBuffer inputBuffer, InFlightRequest* inflightReq,
                 uint32_t frameNumber,
                 bool bEnableTuningData,
                 TestROIParam mTestROIParam);
  bool flush();
  void close();
  std::string getDeviceType();

 private:
  void setMetadata(::android::hardware::hidl_vec<uint8_t> src,
                   uint32_t tag,
                   uint32_t entry_capacity,
                   uint32_t data_capacity,
                   int32_t cnt,
                   void* value);
  void updateMetadata(::android::hardware::hidl_vec<uint8_t> src,
                      uint32_t tag,
                      size_t data_type,
                      size_t data_cnt,
                      void* data);

 private:
  ::android::sp
    <::android::hardware::camera::device::V3_6::ICameraDeviceSession> session;
  android::sp<Hal3Plus> pHal3Plus;
  ::android::hardware::hidl_vec<uint8_t> settings;
  ::android::hardware::camera::device::V3_6::HalStreamConfiguration
    halStreamConfig;
  const char* camDevice3HidlLib = "libmtkcam_hal_hidl_device.so";
  void* mLibHandle = NULL;
  ::android::sp< NSCam::IExtCameraDeviceSession> mExtCameraDeviceSession;
  typedef NSCam::IExtCameraDeviceSession*
      CreateExtCameraDeviceSessionInstance_t(int32_t);
};

class Hal3Plus::extCallback:
    public  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback,
    virtual android::RefBase {
 public:
  explicit extCallback(android::sp<Hal3Plus> pHal3);
  Return<void> notify(const hidl_vec<NotifyMsg>& msgs);
  Return<void> processCaptureResult_3_4(
    const hidl_vec< ::android::hardware::camera::device::V3_4::CaptureResult>&
        results);
  bool processCaptureResultLocked(const CaptureResult& results,
        hidl_vec<PhysicalCameraMetadata> physicalCameraMetadata);
  Return<void> requestStreamBuffers(
    const hidl_vec< ::android::hardware::camera::device::V3_5::BufferRequest>&
        bufReqs,
    ::android::hardware::camera::device::V3_5::ICameraDeviceCallback::\
        requestStreamBuffers_cb _hidl_cb);
  Return<void> returnStreamBuffers(
        const hidl_vec<StreamBuffer>& buffers);
  Return<void> processCaptureResult(
        const hidl_vec<CaptureResult>& results);

 private:
  bool hasOutstandingBuffersLocked();

 private:
  android::sp<Hal3Plus> pHal3Plus;
  hidl_vec< ::android::hardware::camera::device::V3_4::Stream> mStreams;
  hidl_vec< ::android::hardware::camera::device::V3_2::HalStream> mHalStreams;
  using OutstandingBuffers = std::unordered_map<uint64_t, hidl_handle>;
  std::vector<OutstandingBuffers> mOutstandingBufferIds;
  std::condition_variable mFlushedCondition;
  bool mUseHalBufManager = false;
};

class Hal3Plus::nativeInterface :public android::RefBase{
 public:
  explicit nativeInterface(android::sp<Hal3Plus> pHal3);

 public:
  void createNativeDevice();
  void configureNativeDevice(std::vector<int32_t> SensorIdSet,
                             std::vector<Stream> InputStreams,
                             std::vector<Stream> OutputStreams,
                             camera_metadata *pConfigureParams,
                             std::shared_ptr<nativeCallback> pDeviceCallback);
  void processCapture(std::shared_ptr<ProcRequest> request);
  void close();

 private:
  android::sp<Hal3Plus> pHal3Plus;
  std::shared_ptr<INativeDev> pDevice;
  std::shared_ptr<nativeCallback> cb;
};

class Hal3Plus::nativeCallback:public INativeCallback,
                               public android::RefBase {
 public:
  explicit nativeCallback(android::sp<Hal3Plus> pHal3);
  int32_t  processCompleted(int32_t const& status, Result const& result);
  int32_t  processPartial(int32_t const& status,
                          Result const& result,
                          std::vector<BufHandle*> const& completedBuf);

 private:
  android::sp<Hal3Plus> pHal3Plus;
};


#endif  // MAIN_MTKHAL_HIDL_PROVIDER_2_X_TEST_TEST_H_
