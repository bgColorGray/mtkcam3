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
#ifndef MAIN_MTKHAL_CUSTOM_UT_UNITTEST_H_
#define MAIN_MTKHAL_CUSTOM_UT_UNITTEST_H_

#include <inttypes.h>
#include <semaphore.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDevice.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceCallback.h"
#include "mtkcam3/main/mtkhal/core/device/3.x/IMtkcamDeviceSession.h"
#include "mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h"
#include "mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProviderCallback.h"
#include "mtkcam/utils/std/Format.h"
#include "mtkcam/def/common.h"
#include "main/mtkhal/core/utils/imgbuf/include/IIonImageBufferHeap.h"
#include "mtkcam/utils/metadata/IMetadata.h"
#include "utils/KeyedVector.h"
#include "utils/Thread.h"
#include "utils/Timers.h"
#include "utils/Vector.h"

using mcam::BufferStatus;
using mcam::CameraBlob;
using mcam::CameraBlobId;
using mcam::CaptureRequest;
using mcam::CaptureResult;
using mcam::Dataspace;
using mcam::ErrorCode;
using mcam::HalStreamConfiguration;
using mcam::IMtkcamDevice;
using mcam::IMtkcamDeviceCallback;
using mcam::IMtkcamDeviceSession;
using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;
using mcam::MsgType;
using mcam::NotifyMsg;
using mcam::RequestTemplate;
using mcam::Status;
using mcam::Stream;
using mcam::StreamBuffer;
using mcam::StreamConfiguration;
using mcam::StreamConfigurationMode;
using mcam::StreamType;
using mcam::VendorTagSection;

using NSCam::EImageFormat;
using mcam::IImageBufferHeap;
using NSCam::IMetadata;
using NSCam::MPoint;
using NSCam::MRect;
using NSCam::MSize;

using std::string;

enum testScenario {
  BASIC_YUV_STREAM,          // 1 yuv
  BASIC_RAW_STREAM,          // 1 raw
  BASIC_YUV_VARIO_ROI,       // diff ROI
  BASIC_YUV_VARIO_DMA_PORT,  // diff DMA
  MULTICAM_PHY_YUV,          // 2 yuv stream
  MULTICAM_SWITCH_SENSOR,    // 2 yuv stream with one error
  MCNR_F0,                   // f0 + isp tuning
  MCNR_F0_F1,                // f0 + f1 + isp tuning
  MCNR_F0_F1_ME,             // f0 + f1 + ME + isp tuning
  MCNR_F0_F1_ME_WPE,         // f0 + f1 + ME + isp tuning + WPE map
  WPE_PQ,                    // WPE + pq hint
  FD_YUV_STREAM,             // 1 yuv
  MSNR_RAW_YUV,
};
/*
struct JsonSetting {
  std::string sMainName;
};
*/
struct config {
  std::vector<std::string> inputStreams;
  std::vector<std::string> outputStreams;
  std::vector<std::string> metadata;
};
struct request {
  std::vector<std::string> info;
};
struct input {
  std::vector<int32_t> id;
};

struct output {
  std::vector<int32_t> id;
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

struct MetadataSetting {
  uint32_t tagId = 0;
  string tagName = "";
  string tagType = "";
  std::vector<uint32_t> tagValues;
  std::vector<string> tagValuesName;
};

struct StreamSetting {
  int32_t streamId = -1;
  int32_t format = 0;
  int32_t width = 0;
  int32_t height = 0;
  int32_t stride = 0;
  int32_t usage = 0;
  int32_t dataSpace = 0;
  int32_t transform = 0;
  int32_t physicalCameraId = -1;
  std::vector<MetadataSetting> settings;
};

struct ConfigSetting {
  std::vector<StreamSetting> inputStreams;
  std::vector<StreamSetting> outputStreams;
  std::vector<MetadataSetting> sessionParams;
};

struct RequestStream {
  int32_t streamId = -1;
  std::vector<MetadataSetting> streamSettings;
};

struct RequestSetting {
  std::vector<int32_t> frameNo;
  std::vector<RequestStream> inputStreams;
  std::vector<RequestStream> outputStreams;
  std::vector<MetadataSetting> appMetadatas;
  std::vector<MetadataSetting> halMetadatas;
};

struct ConfigAndRequestSetting {
  ConfigSetting configSetting;
  std::vector<RequestSetting> requestSettings;
};

struct JsonData {
  string filePath = "";
  string keyName = "";
  string caseName = "";
  string caseType = "";
  int32_t cameraDeviceId = -1;
  int32_t rhsDeviceId = -1;
  uint32_t operationMode = 0;
  int32_t streamId_ME = -1;
  int32_t streamId_TME = -1;
  int32_t streamId_warpMap = -1;
  bool useWPEtoCrop = 0;
  ConfigAndRequestSetting Lhs;
  ConfigAndRequestSetting Rhs;
};

typedef std::map<int, std::string> JsonTypeMap;
JsonTypeMap g_JsonTypeMap = {
    {0, "JSon::nullValue"},
    {1, "JSon::intValue"},
    {2, "JSon::uintValue"},
    {3, "JSon::realValue"},
    {4, "JSon::stringValue"},
    {5, "JSon::booleanValue"},
    {6, "JSon::arrayValue"},
    {7, "JSon::objectValue"}};

class MtkHalData {
 public:
  struct AllocateBufferInfo {
    std::shared_ptr<mcam::IImageBufferHeap> pImgBufHeap = nullptr;
    int32_t bufferId = 0;
  };
  struct AllocateBufferSet {
    // <streamId, AllocateBufferInfo>
    std::map<int32_t, AllocateBufferInfo> mLhsOutImgBufferInfos;
    std::map<int32_t, AllocateBufferInfo> mRhsInImgBufferInfos;
    std::map<int32_t, AllocateBufferInfo> mRhsOutImgBufferInfos;
  };
  struct MtkHalDeviceData {
    // config data
    StreamConfiguration configSetting;
    // specific request data <frameNo, CaptureRequest>
    std::map<uint32_t, CaptureRequest> specificRequestsMap;
    // repeat request data
    CaptureRequest repeatRequest;
  };

  MtkHalData() {}
  virtual ~MtkHalData() {}
  bool transformFromJsonData(JsonData const& jsonData, IMetadata& templateMeta);
  void dumpData();
  //
  bool getLhsConfigData(StreamConfiguration& setting);
  bool getRhsConfigData(StreamConfiguration& setting);
  //
  bool getLhsRequestData(int frameNo, CaptureRequest& requests);
  bool getRhsRequestData(int frameNo, CaptureRequest& requests);
  bool allocateStreamBuffer();
  bool getAllocateBufferInfo_LhsOutput(int32_t frameNo,
                                       int32_t streamId,
                                       AllocateBufferInfo& bufInfo);
  bool getAllocateBufferInfo_RhsInput(int32_t frameNo,
                                      int32_t streamId,
                                      AllocateBufferInfo& bufInfo);
  bool getAllocateBufferInfo_RhsOutput(int32_t frameNo,
                                       int32_t streamId,
                                       AllocateBufferInfo& bufInfo);
  void writeWarpMap(StreamBuffer& buf, int32_t imgW, int32_t imgH);
  void writeWarpMap(StreamBuffer& buf, int32_t imgW, int32_t imgH,
                    int64_t wpeOutW, int64_t wpeOutH);

  MtkHalDeviceData lhsData;
  MtkHalDeviceData rhsData;

 private:
  bool allocateIonBuffer(Stream& srcStream, AllocateBufferInfo* dstBufferInfo);
  bool isValid();
  bool mbTransformed = false;
  IMetadata mTemplateSetting;

  // <index, AllocateBufferSet>
  std::map<int32_t, AllocateBufferSet> mAllocateBufferSets;
  int mBufNum = 10;
  int32_t mStreamId_TME;
  int32_t mStreamId_ME;
  int32_t mStreamId_warpMap;
  bool mUseWPEtoCrop;
};

class JSonUtil {
 public:
  enum ParseResult {
    SUCCESSFUL = 0,
    KEY_NAME_NOT_MATCH,
    OPEN_FILE_FAILED,
    MUST_FILLED_INFO_NOT_FOUND,
    NOT_EXPECTED_TYPE
  };

  JSonUtil() { return; }
  virtual ~JSonUtil() { return; }

  ParseResult ParseJsonFile(const char* filePath, JsonData& jsonData);
  static void dumpJsonData(JsonData const& jsonData);
};

class JsonParser {
 public:
  JsonParser();
  virtual ~JsonParser();
  void setJsonPath(std::string jsonPath) { mJSONFilePath = jsonPath; }
  int parseJson(bool const& bEnableNativeDevice,
                bool const& bEnableTuningData,
                enum testScenario const& runCase,
                int& openId,
                std::string& deviceType,
                IMetadata& lhsConfigSetting,
                IMetadata& rhsConfigSetting,
                IMetadata& lhsRequestSetting,
                IMetadata& rhsRequestSetting,
                std::vector<Stream>& lhsConfigOutputStreams,
                std::vector<Stream>& rhsConfigInputStreams,
                std::vector<Stream>& rhsConfigOutputStreams,
                std::vector<struct req>& lhsRequestIds,
                std::vector<struct req>& rhsRequestIds,
                int32_t& meStreamId);

 private:
  int32_t mDebugLevel;
  std::string mJSONFilePath;
};

struct InFlightRequest {
  // A 64bit integer to index the frame number associated with this result.
  int64_t frameNumber = 0;

  // For buffer drop errors, the stream ID for the stream that lost a buffer.
  // For physical sub-camera result errors, the Id of the physical stream
  // for the physical sub-camera.
  // Otherwise -1.
  int32_t errorStreamId = -1;

  /////////////////////////////////////////////////////////////
  // follows are added in Notify()
  bool errorCodeValid = false;
  ErrorCode errorCode = ErrorCode::ERROR_DEVICE;

  // Set by notify() SHUTTER call.
  nsecs_t shutterTimestamp = 0;

  /////////////////////////////////////////////////////////////
  // follows are added in processCaptureResult()

  // Result metadata
  IMetadata fullAppResult;
  IMetadata fullHalResult;

  // Result Output Buffers
  std::vector<StreamBuffer> resultOutputBuffers;

  // Has last result metadata
  bool haveLastResultMetadata = false;
  bool receivedInputBuffer = false;
  bool receivedOutputBuffer = false;
  int inputBufferNum = 0;
  int outputBufferNum = 0;
};

class CmdOptionParser {
 public:
  CmdOptionParser() {}
  void setCmdOption(const std::vector<std::string>& options) {
    mOptions = options;
  }
  const std::string& getCmdOption(const std::string& option) const {
    std::vector<std::string>::const_iterator iter;
    iter = std::find(mOptions.begin(), mOptions.end(), option);
    if (iter != mOptions.end() && ++iter != mOptions.end()) {
      return *iter;
    }
    static const std::string emptyString("");
    return emptyString;
  }
  bool isOptionExists(const std::string& option) const {
    return std::find(mOptions.begin(), mOptions.end(), option) !=
           mOptions.end();
  }

 private:
  std::vector<std::string> mOptions;
};

class MtkHalTest {
 public:
  struct InFlightRequest {
    uint32_t frameNumber = 0;
    int outputBufferNum = 0;

    // Result metadata
    IMetadata fullAppResult;
    IMetadata fullHalResult;

    // Result Output Buffers
    std::vector<StreamBuffer> resultOutputBuffers;

    // Has last result metadata
    bool haveLastResultMetadata = false;
    bool receivedAllOutputBuffer = false;
  };

  struct CallbackListener : public IMtkcamDeviceCallback {
   public:
    explicit CallbackListener(MtkHalTest* pParent, bool bIsLhs);

    virtual auto processCaptureResult(const std::vector<CaptureResult>& results)
        -> void;

    virtual auto notify(const std::vector<NotifyMsg>& msgs) -> void;

    static auto create(MtkHalTest* pParent, bool bIsLhs) -> CallbackListener*;

    MtkHalTest* mParent;
    bool mbIsLhs;
  };

  MtkHalTest() {}
  virtual ~MtkHalTest() {}
  void start(const std::shared_ptr<IPrinter> printer,
             const std::vector<std::string>& options,
             const std::vector<int32_t>& cameraIdList);

  bool LoadJsonFile(string const& sPath);
  void FlowSettingPolicy();
  bool processResult(const mcam::CaptureResult& oneResult,
                     std::map<uint32_t, InFlightRequest>& inFlightTable,
                     std::string& msg);
  void LhsNotify(const std::vector<mcam::NotifyMsg>& messages);
  void LhsProcessCompleted(
      const std::vector<mcam::CaptureResult>& results);
  bool LhsProcessResult(const mcam::CaptureResult& oneResult);
  void RhsNotify(const std::vector<mcam::NotifyMsg>& messages);
  void RhsProcessCompleted(
      const std::vector<mcam::CaptureResult>& results);
  bool RhsProcessResult(const mcam::CaptureResult& oneResult);
  void dumpStreamBuffer(const StreamBuffer& streamBuf,
                        std::string prefix_msg,
                        uint32_t frameNumber);

  MtkHalData mMtkHalData;
  JsonData mJsonData;
  CmdOptionParser mCmdOptionParser;
  //
  bool mEnableRhsFlow = true;
  bool mbEnableDump = true;
  RequestTemplate mTemplateType = RequestTemplate::PREVIEW;
  string mTestCaseType;
  int mCameraDeviceId = 0;
  int mRhsDeviceId = 0;
  int mRequestNum = 0;

  std::mutex mLock;
  std::condition_variable mCondition;
  bool mbWaitLhsDone = false;
  bool mbWaitRhsDone = false;

  std::map<uint32_t, InFlightRequest> mLhsInFlightRequestTable;
  std::map<uint32_t, InFlightRequest> mRhsInFlightRequestTable;
};

class MtkHalUt : public ::android::Thread {
 public:
  typedef std::map<uint32_t,
                   std::shared_ptr<InFlightRequest>>  // <frameNumber,
                                                      // InFlightRequest>
      InFlightMap;
  struct CallbackListener : public IMtkcamDeviceCallback {
   public:
    explicit CallbackListener(MtkHalUt* pParent, bool bIsLhs);

    virtual auto processCaptureResult(const std::vector<CaptureResult>& results)
        -> void;

    virtual auto notify(const std::vector<NotifyMsg>& msgs) -> void;

    static auto create(MtkHalUt* pParent, bool bIsLhs) -> CallbackListener*;

    MtkHalUt* mParent;
    bool mbIsLhs;
  };

 public:
  MtkHalUt();
  virtual ~MtkHalUt();
  void onLastStrongRef(const void* id);

 public:
  bool initial();
  void start(const std::shared_ptr<IPrinter> printer,
             const std::vector<std::string>& options,
             const std::vector<int32_t>& cameraIdList);
  // int initial(std::string options, std::vector<std::string> devices);
  int getOpenId();
  void setDevice(::std::shared_ptr<IMtkcamDevice> cameraDevice);
  void setVendorTag(std::vector<VendorTagSection> vndTag);
  virtual auto requestExit() -> void;
  virtual auto readyToRun() -> android::status_t;

 private:
  virtual auto threadLoop() -> bool;

 private:
  int config();
  int flush();
  int decideLhsTargetNum(std::vector<struct req>& reqs);
  int decideRhsTargetNum(std::vector<struct req>& reqs);
  int parseOptions(std::string options);
  void allocateIonBuffer(Stream srcStream, StreamBuffer* dstBuffers);
  int allocateStreamBuffers();
  int constructExtRequest();
  int constructNativeRequest();
  // bool getCharacteristics();
  int queueRequest();
  bool processResult(const mcam::CaptureResult& oneResult,
                     InFlightMap& inflightMap,
                     std::string& msg);
  void LhsNotify(const std::vector<mcam::NotifyMsg>& messages);
  void LhsProcessCompleted(
      const std::vector<mcam::CaptureResult>& results);
  bool LhsProcessResult(const mcam::CaptureResult& oneResult);
  void RhsNotify(const std::vector<mcam::NotifyMsg>& messages);
  void RhsProcessCompleted(
      const std::vector<mcam::CaptureResult>& results);
  bool RhsProcessResult(const mcam::CaptureResult& oneResult);
  void dumpStreamBuffer(const StreamBuffer& streamBuf,
                        std::string prefix_msg,
                        uint32_t frameNumber);
  bool isReadyToExit();

 private:
  std::vector<struct req> mLhsReqs;
  std::vector<struct req> mRhsReqs;
  int mOpenId;
  int mInstanceId;
  int mTotalRequestNum;
  std::string mDeviceType;
  enum testScenario mRunCase;
  bool mbEnableTuningData;
  bool mbEnableNativeDevice;
  bool mbEnableDump;

  //
  std::shared_ptr<IMtkcamProvider> mLhsProvider;
  std::shared_ptr<IMtkcamDevice> mLhsDevice;
  std::shared_ptr<IMtkcamDeviceSession> mLhsSession;
  std::shared_ptr<CallbackListener> mLhsListener;
  //
  std::shared_ptr<IMtkcamProvider> mRhsProvider;
  std::shared_ptr<IMtkcamDevice> mRhsDevice;
  std::shared_ptr<IMtkcamDeviceSession> mRhsSession;
  std::shared_ptr<CallbackListener> mRhsListener;
  //
  IMetadata mCameraCharacteristics;
  //
  mcam::StreamConfigurationMode mOperationMode;
  RequestTemplate mLhsRequestTemplate;
  RequestTemplate mRhsRequestTemplate;
  int mMaxBuffers;
  int mLhsTargetNum;
  int mRhsTargetNum;
  std::mutex mLhsLock;
  std::mutex mRhsLock;
  std::vector<int> mLhsReceivedFrameNumber;
  std::vector<int> mRhsReceivedFrameNumber;
  std::condition_variable mLhsCondition;
  std::condition_variable mRhsCondition;
  std::vector<Stream> mLhsConfigOutputStreams;
  std::vector<Stream> mRhsConfigOutputStreams;
  std::vector<Stream> mRhsConfigInputStreams;
  IMetadata mLhsRequestSetting;
  IMetadata mRhsRequestSetting;
  IMetadata mLhsConfigSetting;
  IMetadata mRhsConfigSetting;

  std::vector<StreamBuffer>
      mLhsAllocateOutputBuffers;  // ALL = mMaxBuffers * out stream number
  std::vector<StreamBuffer> mRhsAllocateOutputBuffers;  // All = mMaxBuffers
  // allocate buffer for warp map
  std::vector<StreamBuffer> mRhsAllocateInputBuffers;

  std::map<int, std::vector<StreamBuffer>>
      mLhsConfigOutputBufMap;  // <stream id, buffers vector (size: maxBuffers)>
  std::map<int, std::vector<StreamBuffer>>
      mRhsConfigOutputBufMap;  // <stream id, buffers vector (size: maxBuffers)>
  std::map<int, std::vector<StreamBuffer>>
      mRhsConfigInputBufMap;  // <stream id, buffers vector (size: maxBuffers)>
  std::map<int, std::vector<StreamBuffer>>
      mLhsTotalAvailableBufMap;  // <idx(0~maxBuffers), buffers
                                 // vector(size:stream number)>  //ring buffer
                                 // map
  std::map<int, std::vector<StreamBuffer>>
      mLhsEnquedBufMap;  // <frameNumber, buffers vector(size:stream number)>
                         // //ring buffer map
  std::map<int, std::vector<StreamBuffer>>
      mRhsTotalAvailableBufMap;  // <idx(0~maxBuffers), buffers
                                 // vector(size:stream number)>  //ring buffer
                                 // map
  std::map<int, std::vector<StreamBuffer>>
      mRhsEnquedBufMap;  // <frameNumber, buffers vector(size:stream number)>
                         // //ring buffer map

  InFlightMap mLhsInflightMap;
  InFlightMap mRhsInflightMap;
  // InFlightMap mLhsKeepTempInflightMap;

  int mDebugLevel;
  std::shared_ptr<IPrinter> mPrinter;
  std::vector<std::string> mOptions;
  std::vector<int32_t> mCameraIdList;

  std::string mJSONFilePath;

  int32_t mMeStreamId = -1;
};

// implemented by custom zone library: libmtkcam_hal_custom.so
extern "C" void runMtkHalTest(const std::shared_ptr<IPrinter> printer,
                              const std::vector<std::string>& options,
                              const std::vector<int32_t>& cameraIdList);
extern "C" void runMtkHalUt(const std::shared_ptr<IPrinter> printer,
                            const std::vector<std::string>& options,
                            const std::vector<int32_t>& cameraIdList);

#endif  // MAIN_MTKHAL_CUSTOM_UT_UNITTEST_H_
