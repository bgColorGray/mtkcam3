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
#include <test.h>

#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <linux/ion_drv.h>
#include <linux/ion_4.12.h>
#include <ui/gralloc_extra.h>
#include <vndk/hardware_buffer.h>
#include <linux/mman-proprietary.h>
#include <faces.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/LogTool.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/metadata/IMetadataTagSet.h>


#include <android/hardware/camera/device/3.2/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.3/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.6/ICameraDeviceSession.h>

#include <string>
#include <thread>
#include <fstream>
#include <iostream>
#include <utility>
#include <map>
#include <future>
#include <memory>
#include <vector>

#include "mtkcam/utils/metadata/client/mtk_metadata_tag.h"



#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

Hal3Plus:: Hal3Plus()
: openId(0),
  runTimes(30),
  runCase(BASIC_YUV_STREAM),
  P1DmaCase(-1),
  bEnableTuningData(false),
  bEnableNativeDevice(false) {
  MY_LOGI("construct Hal3Plus");
  mAllocator = IAllocator::getService();
  mMapper = IMapper::getService();
  bEnableDump = ::property_get_int32("vendor.debug.hal3ut.enable.dump", 1);
}

Hal3Plus::~Hal3Plus() {
  requestExit();
  for (int  i = 0; i < extInflightMap.size(); i++) {
    auto inflightReq = extInflightMap.editValueAt(i);
    delete inflightReq;
    extInflightMap.removeItemsAt(i);
  }
  for (int i=0; i < maxBuffers; i++) {
    android::GraphicBufferAllocator::get().free(
             extOutputBuffers[i].buffer.getNativeHandle());
    if (bEnableTuningData) {
      android::GraphicBufferAllocator::get().free(
               ispTuningBuffers[i].buffer.getNativeHandle());
    }
    if (bEnableNativeDevice) {
      android::GraphicBufferAllocator::get().free(
               nativeOutputBuffers[i].buffer.getNativeHandle());
    }
  }
}

int Hal3Plus::initial(std::string options, std::vector<std::string> devices) {
  parseOptions(options);
  parseConfigFile();
  getStreamInfo();

  int num = 0;
  for (auto r : hal3reqs) {
    MY_LOGI("hal3reqs size:%lu", hal3reqs.size());
    auto ids = r.reqOutput.id;
    MY_LOGI("ids size:%lu", ids.size());
    if (runCase == MCNR_F0_F1_ME &&  r.reqOutput.id.size() == 4) {
      MY_LOGI("find the MCNR_F0_F1_ME request");
      break;
    }
    if (runCase == MCNR_F0_F1 &&  r.reqOutput.id.size() == 3) {
      MY_LOGI("find the MCNR_F0_F1 request");
      break;
    }
    if (runCase == MCNR_F0 && r.reqOutput.id.size() == 2) {
      MY_LOGI("find the  MCNR_F0 request");
      break;
    }
    if (runCase == WPE_PQ && r.reqOutput.id.size() == 2) {
      MY_LOGI("find the  WPE_PQ request");
      break;
    }
    if (runCase == FD_YUV_STREAM && r.reqOutput.id.size() == 1) {
      MY_LOGI("find basic fdyuv stream request");
      break;
    }
    if (runCase == MSNR_RAW_YUV &&  r.reqOutput.id.size() == 2) {
      MY_LOGI("find the  MSNR_RAW_YUV request");
      break;
    }
    if (r.reqOutput.id.size() == 1) {
      for (auto vec : extOutputStreams) {
        int streamId = vec.v3_2.id;
        if (runCase == BASIC_YUV_STREAM &&
            vec.v3_2.format == PixelFormat::IMPLEMENTATION_DEFINED &&
            streamId == (r.reqOutput.id[0]).u8) {
          MY_LOGI("find basic yuv stream request");
          break;
        }
        if (runCase == BASIC_RAW_STREAM &&
            vec.v3_2.format == PixelFormat::RAW16 &&
            streamId == (r.reqOutput.id[0]).u8) {
          MY_LOGI("find basic raw stream request");
          break;
        }
      }
    }
    num++;
  }
  targetNum = num;
  deviceNameList = devices;
  std::size_t foundId = deviceNameList[openId].find_last_of("/\\");
  intanceId = atoi((deviceNameList[openId].substr(foundId+1)).c_str());
  MY_LOGD("intance id: %d", intanceId);

  mExtHandler = new extInterface(this);
  mExtCallbackHanlder = new extCallback(this);
  if (bEnableNativeDevice) {
    mNativeHandler = new nativeInterface(this);
    mNativeCallbackHandler = std::shared_ptr<nativeCallback>(
        new nativeCallback(this), [](nativeCallback* p) {delete p;});
  }
  return 0;
}

int Hal3Plus::getOpenId() {
  return openId;
}

void Hal3Plus::setDevice(
  ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice>
  cameraDevice ) {
  auto castResult =
    ::android::hardware::camera::device::V3_6::ICameraDevice::castFrom
       (cameraDevice);
  pCameraDevice = castResult;
}

void Hal3Plus::setVendorTag(hidl_vec<VendorTagSection>  vndTag) {
  mVendorTagSections = vndTag;
}

int Hal3Plus::parseOptions(std::string options) {
  auto start = options.find("-count");
  auto end = options.find(" ");
  if (start !=std::string::npos && end != std::string::npos && start !=end) {
    auto str = options.substr(start+7, end-start-7);
    runTimes = std::stoi(str);
  }
  auto pos = options.find("-t");
  if (pos != std::string::npos) {
    auto subStr = options.substr(pos+3, 3);
    if (subStr == "0_0") {
      runCase = BASIC_YUV_STREAM;
    }
    if (subStr == "0_1") {
      runCase = BASIC_RAW_STREAM;
    }
    if (subStr == "0_2") {
      runCase = BASIC_YUV_VARIO_ROI;
    }
    if (subStr == "1_0") {
      runCase = MCNR_F0;
    }
    if (subStr == "1_2") {
      runCase = MCNR_F0_F1;
    }
    if (subStr == "1_3") {
      runCase = MCNR_F0_F1_ME;
    }
    if (subStr == "2_1") {
      runCase = WPE_PQ;
    }
    if (subStr == "3_1") {
      runCase = FD_YUV_STREAM;
    }

    if (subStr == "7_0") {
      runCase = MSNR_RAW_YUV;
    }
    // For p1 dma ut case
    subStr = options.substr(pos+3, 5);
    if (subStr == "0_3_0") {
      // test YuvR1 + YuvR2 + ME (use MCNR_F0_F1_ME case)
      runCase = MCNR_F0_F1_ME;
      P1DmaCase = 0;
      MY_LOGD("test P1 dma: YuvR1 + YuvR2 + ME");
    }
    if (subStr == "0_3_1") {
      // test YuvR3 + YuvR4 (use MCNR_F0_F1 case with modification)
      runCase = MCNR_F0_F1;
      P1DmaCase = 1;
      MY_LOGD("test P1 dma: YuvR3 + YuvR4");
    }
    if (subStr == "0_3_2") {
      // test Raw (use MCNR_f0 case with modification)
      runCase = MCNR_F0;
      P1DmaCase = 2;
      MY_LOGD("test P1 dma: Raw");
    }
    if (subStr == "0_3_3") {
      // test Depth (use MCNR_f0 case with modification)
      runCase = MCNR_F0;
      P1DmaCase = 3;
      MY_LOGD("test P1 dma: Depth");
    }
  }
  if (runCase == MCNR_F0 ||
      runCase == MCNR_F0_F1 ||
      runCase == MCNR_F0_F1_ME ||
      runCase == MCNR_F0_F1_ME_WPE ||
      runCase == WPE_PQ ||
      runCase == MSNR_RAW_YUV) {
    bEnableTuningData = 1;
    bEnableNativeDevice = 1;
  } else if (runCase == FD_YUV_STREAM) {
    bEnableNativeDevice = 1;
  }
  MY_LOGI("runTimes %d, runCase:%d", runTimes, runCase);
  return 0;
}

int Hal3Plus::parseConfigFile() {
  std::ifstream fd;
  std::string str;
  struct pairSymbol {
    int beg;
    int end;
  };
  std::vector<struct pairSymbol> pairVec;
  int lineNum = 0;
  int hal3StartLine = 0;
  int hal3ConfigLine = 0;
  int hal3RequestLine = 0;
  int ispStartLine = 0;
  int ispConfigLine = 0;
  int ispRequestLine = 0;

  std::vector<std::string> strVec;
  std::map<int, std::string> symbolStrMap;
  std::map<int, std::string> fileStr;
  std::map<int, int> pairMap;

  fd.open("/data/vendor/camera_dump/ut.json", std::ios::in);
  if (fd.is_open()) {
    while (std::getline(fd, str)) {
      strVec.push_back(str);
      lineNum++;
      fileStr.emplace(lineNum, str);
      if (str.find("cameraId") != std::string::npos) {
        auto start = str.find(": \"");
        auto end   = str.find("\",");
        if (start != end &&
            start != std::string::npos &&
            end != std::string::npos) {
          openId = str[start+3]-'0';
          MY_LOGI("openId:%d", openId);
        }
      }

      if (str.find("HAL3") != std::string::npos) {
        hal3StartLine = lineNum;
      }

      if (str.find("ISP") != std::string::npos) {
        ispStartLine = lineNum;
      }

      if (str.find("type") != std::string::npos) {
        auto beg = str.find("\": ");
        auto end = str.find("\",");
        if (beg != std::string::npos &&
            end != std::string::npos &&
            beg != end) {
          deviceType = str.substr(beg+4, (end-beg-4));
          MY_LOGI("deviceType:%s", deviceType.c_str());
        }
      }

      if (str.find("config") != std::string::npos) {
        if (hal3StartLine!= 0 &&
            lineNum > hal3StartLine &&
            ispStartLine == 0) {
          hal3ConfigLine = lineNum;
        }
        if (ispStartLine!= 0 && lineNum > ispStartLine) {
            ispConfigLine = lineNum;
        }
      }

      if (str.find("requests") != std::string::npos) {
        if (hal3StartLine != 0 &&
            lineNum > hal3StartLine &&
            ispStartLine == 0) {
          hal3RequestLine = lineNum;
        }
        if (ispStartLine != 0 && lineNum > ispStartLine) {
          ispRequestLine = lineNum;
        }
      }
      if (str.find("{") != std::string::npos &&
          str.find("}") != std::string::npos) {
        symbolStrMap.emplace(lineNum, "{}");
        str.clear();
        continue;
      }
      if (str.find("{") != std::string::npos) {
        symbolStrMap.emplace(lineNum, "{");
        str.clear();
        continue;
      }
      if (str.find("}") != std::string::npos) {
        symbolStrMap.emplace(lineNum, "}");
        str.clear();
        continue;
      }
    }

    auto iterBeg = symbolStrMap.begin();
    for (auto iter = symbolStrMap.begin(); iter != symbolStrMap.end(); iter++) {
      if (iter->second == "}") {
      auto tmp = iter;
      while (tmp != iterBeg) {
        if (tmp->second == "{") {
          struct pairSymbol temp;
          temp.beg = tmp->first;
          temp.end = iter->first;
          pairVec.push_back(temp);
          pairMap.emplace(tmp->first, iter->first);
          tmp->second = " ";
          break;
        }
        tmp--;
      }
    }
    if (iter->second == "{}") {
      struct pairSymbol temp;
      temp.beg = iter->first;
      temp.end = iter->first;
      pairVec.push_back(temp);
      pairMap.emplace(iter->first, iter->first);
      iter->second = " ";
    }
  }

  auto parseStram = [&] (std::vector<struct pairSymbol>::iterator iter,
                         std::string str,
                         std::string type) mutable {
    auto startline = iter->beg;
    auto endline = iter->end;
    struct config* configType = nullptr;
    std::vector<std::string> *tmp = nullptr;
    if (startline == endline) {
      MY_LOGW("nothing for metadata");
    } else {
      if (type == "hal3") {
        configType = &hal3streamConfig;
      }
      if (type == "isp") {
        configType = &ispstreamConfig;
      }
      if (str == "metadata") {
        tmp = &(configType->metadata);
      }
      if (str == "outputStreams") {
        tmp = &(configType->outputStreams);
      }
      if (str == "inputStreams") {
        tmp = &(configType->inputStreams);
      }

      for (auto line = startline; line <(endline - 1); line++) {
        tmp->push_back(strVec[line]);
      }
      tmp = nullptr;
    }
  };
  for (auto iter = pairVec.begin(); iter != pairVec.end(); iter++) {
    // parse HAL3
    auto tmp = iter;
    if (iter->beg == hal3ConfigLine) {
      for (; tmp != pairVec.begin(); tmp--) {
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("metadata") != std::string::npos) {
          parseStram(tmp, "metadata", "hal3");
        }
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("outputStreams") != std::string::npos) {
          parseStram(tmp, "outputStreams", "hal3");
        }
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("inputStreams") != std::string::npos) {
          parseStram(tmp, "inputStreams", "hal3");
        }
      }
    }

    if (iter->beg == ispConfigLine) {
      for (; tmp != pairVec.begin(); tmp--) {
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("metadata") != std::string::npos) {
          parseStram(tmp, "metadata", "isp");
        }
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("outputStreams") != std::string::npos) {
          parseStram(tmp, "outputStreams", "isp");
        }
        if (tmp->beg > iter->beg &&
            strVec[tmp->beg-1].find("inputStreams") != std::string::npos) {
          parseStram(tmp, "inputStreams", "isp");
        }
      }
    }
  }
  for (auto iter = pairMap.begin(); iter != pairMap.end(); iter++) {
    // parse hal3 request
    auto parseRequest = [&] (std::map<int, int>::iterator it, std::string str) {
      // int reqNum =0;
      std::map<int, int> reqMap;
      auto tmp = it;
      tmp++;
      int start = tmp->first;
      int end   = tmp->second;
      reqMap.emplace(start, end);
      for (auto iter = tmp; iter != pairMap.end(); iter++) {
        if (iter->first > tmp->second && iter->second < it->second) {
          reqMap.emplace(iter->first, iter->second);
          tmp = iter;
        }
      }
      for (auto vec : reqMap) {
        MY_LOGI("start:%d, end:%d", vec.first, vec.second);
      }
      for (auto vec : reqMap) {
        struct request reqTmp;
        for (int line = vec.first; line < vec.second; line++)
          reqTmp.info.push_back(fileStr[line]);
        if (str == "hal3")
          hal3Requests.push_back(reqTmp);
        if (str == "isp")
          ispRequests.push_back(reqTmp);
      }
    };

    if (iter->first == hal3RequestLine) {
      parseRequest(iter, "hal3");
    }
    if (iter->first == ispRequestLine) {
      parseRequest(iter, "isp");
    }
    MY_LOGI("hal3Requests size:%lu, ispRequests size:%lu", hal3Requests.size(),
            ispRequests.size());
    }
    fd.close();
  } else {
    MY_LOGE("cannot open the config file");
    return 1;
  }
  return 0;
}

bool Hal3Plus::getCharacteristics() {
  auto ret = pCameraDevice->getCameraCharacteristics(
             [&](Status s, CameraMetadata metadata) {
    if (s == Status::OK) {
      cameraCharacteristics = metadata;
    }
  });
  ret = pCameraDevice->getPhysicalCameraCharacteristics(deviceNameList[openId],
        [&](auto status, const auto& chars) {
        if (status == Status::OK) {
          MY_LOGI("get %d", openId);
          physicalCharacteristics = chars;
        }
  });
  camera_metadata_t *staticMetadata;
  staticMetadata = clone_camera_metadata(
    reinterpret_cast<const camera_metadata_t*>(cameraCharacteristics.data()));
  camera_metadata_ro_entry entry;
  auto stat = find_camera_metadata_ro_entry(staticMetadata,
    ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
  if ((0 == stat) && (entry.count > 0)) {
    partialResultCount = entry.data.i32[0];
    supportsPartialResults = (partialResultCount > 1);
    MY_LOGI("partialResultCount:%d, supportsPartialResults:%d",
             partialResultCount,
             supportsPartialResults);
  }
  return true;
}

bool Hal3Plus::getStreamInfo() {
  Return<void> ret;
  auto parseInfo = [] (std::vector<std::string> srcVec,
       ::android::hardware::hidl_vec
                 <::android::hardware::camera::device::V3_4::Stream>* stream) {
  int num = 0;
  for (auto str : srcVec) {
    if (str.find("id") != std::string::npos) {
      num++;
    }
  }
  stream->resize(num);
  int32_t streamId = 0;
  auto getSubStr = [&] (std::string& src, std::string name,
                        std::string delimiter, int len,
                        std::string& subStr) {
    subStr.clear();
    auto start = src.find(name);
    auto end = src.find(delimiter);
    if (start != std::string::npos &&
        end != std::string::npos &&
        start !=end) {
      subStr = src.substr(start+len, (end-start-len));
      src[end] = ' ';
    }
  };
  int streamNum = 0;
  for (auto vec : srcVec) {
    std::string tmpStr;
    getSubStr(vec, "\"id\"", ", ", 5, tmpStr);
    streamId = std::stoi(tmpStr);
    getSubStr(vec, "\"width\"", ", ", 8, tmpStr);
    auto width = std::stoi(tmpStr);
    getSubStr(vec, "\"height\"", ", ", 9, tmpStr);
    auto height = std::stoi(tmpStr);
    getSubStr(vec, "\"format\"", ", ", 9, tmpStr);
    auto format = std::stoi(tmpStr);
    getSubStr(vec, "\"phyId\"", ", ", 8, tmpStr);
    auto physicalId = tmpStr.c_str();
    getSubStr(vec, "\"usage\"", ",",  8, tmpStr);
    auto usage1 = std::stoi(tmpStr);
    getSubStr(vec, "\"size\"", ",", 7, tmpStr);
    auto bufSize = std::stoi(tmpStr);
     ::android::hardware::camera::device::V3_4::Stream stream3_4 = {{streamId,
        StreamType::OUTPUT,
        static_cast<uint32_t> (width),
        static_cast<uint32_t> (height),
        static_cast<::android::hardware::graphics::common::V1_0::PixelFormat>
                                                                      (format),
        ::android::hardware::hidl_bitfield<
                 ::android::hardware::graphics::common::V1_0::BufferUsage>
                                                                      (usage1),
        0,
        StreamRotation::ROTATION_0},
        physicalId,
        static_cast<uint32_t>(bufSize)};
    (*stream)[streamNum++] = stream3_4;
    MY_LOGI("streamId:%d, width:%d, height:%d, bufSize:%d", streamId, width,
            height, bufSize);
  }
  };
  // ext part
  parseInfo(hal3streamConfig.outputStreams, &extOutputStreams);
  if (bEnableNativeDevice) {
    parseInfo(ispstreamConfig.inputStreams, &nativeInputStreams);
    // TODO(MTK): add judgement nativeInputStreams and extOutputStreams
    parseInfo(ispstreamConfig.outputStreams, &nativeOutputStreams);
  }
  auto getSubStr = [] (std::string str,
                       std::string beg,
                       std::string medium,
                       std::string end,
                       std::vector<data> * vec,
                       int metaType) {
    auto start = str.find(beg);
    auto last   = str.find(end);
    auto mapToValue = [&] (std::string str) {
      data tmp;
      switch (metaType) {
        case 0: {
            int u8 = std::stoi(str);
            tmp.u8 = u8;
            vec->push_back(tmp);
            MY_LOGI("val :%d", u8);
            break;
          }
          case 1: {
            int i32 = std::stoi(str);
            tmp.i32 = i32;
            vec->push_back(tmp);
            MY_LOGI("val :%d", i32);
            break;
          }
          case 2: {
            float  f = std::stof(str);
            tmp.f = f;
            vec->push_back(tmp);
            MY_LOGI("val :%f", f);
            break;
          }
          case 3: {
            int64_t  i64 = std::stol(str);
            tmp.i64 = i64;
            vec->push_back(tmp);
            MY_LOGI("val : (%" PRId64 ") ", i64);
            break;
          }
          case 4: {
            double  d = std::stod(str);
            tmp.d = d;
            vec->push_back(tmp);
            break;
          }
        }
    };
    if (start != std::string::npos &&
        last != std::string::npos  &&
        start != last) {
      auto med = str.find(medium);
      while (med != std::string::npos && med < last) {
        auto subStr = str.substr(start+1, (med-start-1));
        mapToValue(subStr);
        start = med+1;
        str[med] = ' ';
        med = str.find(",");
      }
    }
    auto subStr = str.substr(start+1, (last-start-1));
    mapToValue(subStr);
  };
  auto parseMeta =
    [&] (std::string str, std::vector<struct meta>* vec) {
    auto start = str.find("com.mediatek.");
    auto end   = str.find("\":");
    struct meta metaTmp;
    if (start != std::string::npos &&
        end != std::string::npos   &&
        start != end) {
      std::string metaName = str.substr(start, (end-start));
      str[end] = ' ';
      metaTmp.name = metaName;
    }
    start = str.find("type");
    end   = str.find(",");
    if (start != std::string::npos &&
        end != std::string::npos   &&
        start != end) {
      auto subStr = str.substr(start+7, end-start-7);
      auto metaType = std::stoi(subStr);
      str[end] = ' ';
      metaTmp.type = metaType;
    }
    start = str.find("value");
    end   = str.find("}");
    if (start != std::string::npos &&
        end != std::string::npos   &&
        start != end) {
      std::vector<data> val;
      getSubStr(str, "[", ",", "]", &val, metaTmp.type);
      metaTmp.value = val;
    }
    vec->push_back(metaTmp);
  };
  for (auto vec : hal3streamConfig.metadata) {
    struct meta tmp;
    parseMeta(vec, &sessionConfig);
    // sessionConfig.push_back(tmp);
  }

  for (auto vec : hal3Requests) {
    struct input hal3In;
    struct output hal3Out;
    struct req    reqOne;
    int inputBeg = 0;
    int outputBeg = 0;
    int verify = 0;
    for (auto num = 0; num < vec.info.size(); num++) {
      std::string str = (vec.info)[num];
      if (str.find("inputBuffer") != std::string::npos) {
        inputBeg = num;
        MY_LOGI("inputBeg:%d", inputBeg);
      }
      if (str.find("outputBuffer") != std::string::npos) {
        outputBeg = num;
        MY_LOGI("outputBeg:%d", outputBeg);
      }
      if (str.find("verify") != std::string::npos) {
        verify = num;
      }
      if (str.find("id") != std::string::npos && outputBeg == 0) {
        getSubStr(str, "[", ",", "]", &(hal3In.id), 0);
      }
      if (str.find("com.mediatek.") != std::string::npos && outputBeg == 0) {
        parseMeta(str, &(hal3In.metadata));
      }
      if (str.find("id") != std::string::npos && outputBeg != 0) {
        getSubStr(str, "[", ",", "]", &(hal3Out.id), 0);
      }
      if (str.find("trigger") != std::string::npos &&
          outputBeg != 0 &&
          verify ==0) {
        auto start = str.find(": \"");
        auto end   = str.find("\",");
        auto subStr = str.substr(start+3, end-start-3);
        hal3Out.reqType = subStr;
      }
      if (str.find("com.mediatek.") != std::string::npos &&
          outputBeg != 0 &&
          verify ==0) {
        parseMeta(str, &(hal3Out.metadata));
      }
      if (str.find("com.mediatek.") != std::string::npos &&
          outputBeg != 0 &&
          verify !=0) {
        parseMeta(str, &(hal3Out.checkMeta));
      }
    }
    reqOne.reqInput = hal3In;
    reqOne.reqOutput = hal3Out;
    hal3reqs.push_back(reqOne);
  }
  for (auto vec : hal3reqs) {
    for (auto id : vec.reqInput.id) {
      MY_LOGI("hal3 request input stream id %d ", id.u8);
    }
    for (auto meta : vec.reqInput.metadata) {
      MY_LOGI("hal3 request input stream metadata name %s ",
               meta.name.c_str());
    }
    for (auto id : vec.reqOutput.id) {
      MY_LOGI("hal3 request output stream id:%d ", id.u8);
    }
    for (auto meta : vec.reqOutput.metadata) {
      MY_LOGI("hal3 request input stream metadata name %s, type:%d" ,
              meta.name.c_str(), meta.type);
    }
    MY_LOGI("hal3 request trigger type :%s" , vec.reqOutput.reqType.c_str());
  }
  for (auto vec : ispRequests) {
    struct input ispIn;
    struct output ispOut;
    struct req    reqOne;
    int inputBeg = 0;
    int outputBeg = 0;
    int verify = 0;
    for (auto num = 0; num < vec.info.size(); num++) {
      std::string str = (vec.info)[num];
      if (str.find("inputBuffer") != std::string::npos) {
        inputBeg = num;
      }
      if (str.find("outputBuffer") != std::string::npos) {
        outputBeg = num;
      }
      if (str.find("verify") != std::string::npos) {
        verify = num;
      }
      if (str.find("id") != std::string::npos && outputBeg == 0) {
        std::cout << "find isp in id " << std::endl;
        getSubStr(str, "[", ",", "]", &(ispIn.id), 0);
      }
      if (str.find("com.mediatek.") != std::string::npos && outputBeg == 0) {
        std::cout << "find isp in  metadata " << std::endl;
        parseMeta(str, &(ispIn.metadata));
      }
      if (str.find("id") != std::string::npos && outputBeg != 0) {
        std::cout << "find isp out  id " << std::endl;
        getSubStr(str, "[", ",", "]", &(ispOut.id), 0);
      }
      if (str.find("trigger") != std::string::npos &&
          outputBeg != 0 &&
          verify ==0) {
        auto start = str.find(": \"");
        auto end   = str.find("\",");
        auto subStr = str.substr(start+3, end-start-3);
        ispOut.reqType = subStr;
      }
      if (str.find("com.mediatek.") != std::string::npos &&
          outputBeg != 0 &&
          verify ==0) {
        std::cout << "find isp out  metadata " << std::endl;
        parseMeta(str, &(ispOut.metadata));
      }
      if (str.find("com.mediatek.") != std::string::npos &&
          outputBeg != 0 &&
          verify !=0) {
        parseMeta(str, &(ispOut.checkMeta));
      }
    }
    reqOne.reqInput = ispIn;
    reqOne.reqOutput = ispOut;
    ispreqs.push_back(reqOne);
  }
  MY_LOGI("hal3reqs size:%lu,ispreqs size:%lu",
          hal3reqs.size(),
          ispreqs.size());
  return true;
}

void Hal3Plus::setMeta(
  ::android::hardware::hidl_vec<uint8_t>& src,
  uint32_t tag,
  uint32_t entry_capacity,
  uint32_t data_capacity,
  int32_t cnt,
  void* value) {
  const camera_metadata_t* pMetadata =
                       reinterpret_cast<const camera_metadata_t*> (src.data());
  camera_metadata_t* vendor = nullptr;
  const size_t entryCapacity = (get_camera_metadata_entry_capacity(pMetadata) +
                                entry_capacity);
  const size_t dataCapacity = (get_camera_metadata_data_capacity(pMetadata)+
                               data_capacity);
  vendor = allocate_camera_metadata(entryCapacity, dataCapacity);
  size_t memory_needed = calculate_camera_metadata_size(entryCapacity,
                                                        dataCapacity);

  if (!validate_camera_metadata_structure(vendor, &memory_needed)) {
    if (0!= add_camera_metadata_entry(vendor, tag, value, cnt)) {
      MY_LOGE("add  failed, tag:%d", tag);
    } else {
      MY_LOGD("add  success, tag:%d", tag);
      int ret = append_camera_metadata(vendor, pMetadata);
      if (!ret) {
        MY_LOGI("append session parameter success");
      } else {
        MY_LOGI("append session parameter failed");
      }
      src.setToExternal(reinterpret_cast<uint8_t *> (vendor),
                            get_camera_metadata_size(vendor),
                            true);
    }
  } else {
    MY_LOGE("FAIL");
  }
}


int Hal3Plus::open() {
  // open ext
  mExtHandler->open(pCameraDevice, mExtCallbackHanlder);
  // open native
  if (bEnableNativeDevice) {
    mNativeHandler->createNativeDevice();
  }

  return 0;
}

void Hal3Plus::setMetadata(
  ::android::hardware::hidl_vec<uint8_t> src,
  uint32_t tag,
  uint32_t entry_capacity,
  uint32_t data_capacity,
  int32_t cnt,
  void* value) {
  const camera_metadata_t* pMetadata =
                       reinterpret_cast<const camera_metadata_t*> (src.data());
  camera_metadata_t* vendor = nullptr;
  const size_t entryCapacity = (get_camera_metadata_entry_capacity(pMetadata) +
                                entry_capacity);
  const size_t dataCapacity = (get_camera_metadata_data_capacity(pMetadata)+
                               data_capacity);
  vendor = allocate_camera_metadata(entryCapacity, dataCapacity);
  size_t memory_needed = calculate_camera_metadata_size(entryCapacity,
                                                        dataCapacity);

  if (!validate_camera_metadata_structure(vendor, &memory_needed)) {
    if (0!= add_camera_metadata_entry(vendor, tag, value, cnt)) {
      MY_LOGE("add  failed");
    } else {
      MY_LOGD("add  success");
      int ret = append_camera_metadata(vendor, pMetadata);
      if (!ret) {
        MY_LOGI("append session parameter success");
      } else {
        MY_LOGI("append session parameter failed");
      }
      settings.setToExternal(reinterpret_cast<uint8_t *> (vendor),
                             get_camera_metadata_size(vendor),
                             true);
    }
  } else {
    MY_LOGE("FAIL");
  }
}

int Hal3Plus::config() {
  //getStreamInfo();
  // ext config
  mExtHandler->constructDefaultSetting(
          bEnableTuningData,
          &settings);
  // update settings according to the ut.json
  for (auto meta : sessionConfig) {
    auto end = meta.name.find_last_of(".");
    std::string secName;
    std::string tagName;
    uint32_t tagId;
    if (end != std::string::npos) {
      secName = meta.name.substr(0, end);
      tagName  = meta.name.substr(end+1, meta.name.size()-end-1);
    }
    for (auto vnd : mVendorTagSections) {
      if (vnd.sectionName == secName) {
        auto tags = vnd.tags;
        for (auto tag : tags) {
          if (tag.tagName == tagName) {
            tagId = tag.tagId;
            break;
          }
        }
        break;
      }
    }
    size_t data_used = camera_metadata_type_size[meta.type];
    size_t cnt = (meta.value).size();
    data *val = new data[cnt];
    int i = 0;
    for (auto vec : meta.value) {
      val[i++] = vec;
    }
    setMetadata(settings, tagId, 1, data_used, cnt,
                reinterpret_cast<void *> (val));
    delete[] val;
    MY_LOGI("session meta:%s, type:%d", meta.name.c_str(), meta.type);
  }

  // set ext I/F setting
  auto Ids = hal3reqs[targetNum].reqOutput.id;
  MY_LOGD("hal3plus config input stream num (%d)", Ids.size());
  if (runCase == MCNR_F0) {
    // TODO(MTK): need set case by case
    MY_LOGD("fd sid : %d, tuning sid : %d", Ids[0].u8, Ids[1].u8);
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_DEPTH << 8) | openId;
    int64_t  f0ParamCnt   = 1;
    int64_t  f0Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK;

    int64_t  tuningStreamId   = Ids[1].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    // hardcode: for p1 dma port ut
    if (P1DmaCase == 2) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_FULL_RAW << 8) | openId;
      f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK;
    }
    if (P1DmaCase == 3) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_DEPTH << 8) | openId;
      f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK;
    }
    //
    int64_t value[] = {
        2, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*yv12*/
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, /*physicCameraId:0, real time, streaming, undefined feature, isp tuning, serial number 0 */
        tuningParamCnt, /*format*/
    };
    size_t data_used = camera_metadata_type_size[3];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  } else if (runCase == MCNR_F0_F1) {
    // TODO(MTK): need set case by case
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R1 << 8) | openId;
    int64_t  f0ParamCnt   = 1;
    int64_t  f0Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK;

    int64_t  f1StreamId   = Ids[1].u8;
    int64_t  f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R2 << 8) | openId;
    int64_t  f1ParamCnt   = 1;
    int64_t  f1Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f1Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK;

    int64_t  tuningStreamId   = Ids[2].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    // hardcode: for p1 dma port ut
    if (P1DmaCase == 1) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R3 << 8) | openId;
      f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R4 << 8) | openId;
    }
    // hardcode: use setProperty to control p1 dma port
    bool useR3R4 = 0;
    useR3R4 = ::property_get_int32("vendor.debug.hal3ut.user3r4", 0);
    MY_LOGD("P1 DMA port use R3R4 :%d", useR3R4);
    if (useR3R4 == 1) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R3 << 8) | openId;
      f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R4 << 8) | openId;
    }
    //
    int64_t value[] = {
        3, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*yv12*/
        f1StreamId, /* f0 stream id*/
        f1Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f1ParamCnt, /*param count*/
        f1Setting, /*setting format*/
        f1Format, /*yv12*/
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, /*physicCameraId:0, real time, streaming, undefined feature, isp tuning, serial number 0 */
        tuningParamCnt, /*format*/
    };
    size_t data_used = camera_metadata_type_size[3];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  } else if (runCase == MCNR_F0_F1_ME) {
    // TODO(MTK): need set case by case
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R1 << 8) | openId;
    int64_t  f0ParamCnt   = 1;
    int64_t  f0Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK;

    int64_t  f1StreamId   = Ids[1].u8;
    int64_t  f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R2 << 8) | openId;
    int64_t  f1ParamCnt   = 1;
    int64_t  f1Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f1Format     = MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK;

    int64_t  MEStreamId   = Ids[2].u8;
    int64_t  MEDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_ME << 8) | openId;
    int64_t  MEParamCnt   = 1;
    int64_t  MESetting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  MEFormat     = MTK_HAL3PLUS_STREAM_FORMAT_Y8;

    int64_t  tuningStreamId   = Ids[3].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    // hardcode: use setProperty to control p1 dma port
    bool useR3R4 = 0;
    useR3R4 = ::property_get_int32("vendor.debug.hal3ut.user3r4", 0);
    MY_LOGD("P1 DMA port use R3R4 :%d", useR3R4);
    if (useR3R4 == 1) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R3 << 8) | openId;
      f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R4 << 8) | openId;
    }

    int64_t value[] = {
        4, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*yv12*/
        f1StreamId, /* f1 stream id*/
        f1Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f1ParamCnt, /*param count*/
        f1Setting, /*setting format*/
        f1Format, /*yv12*/
        MEStreamId, /* ME stream id*/
        MEDescriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        MEParamCnt, /*param count*/
        MESetting, /*setting format*/
        MEFormat, /*yv12*/
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, /*physicCameraId:0, real time, streaming, undefined feature, isp tuning, serial number 0 */
        tuningParamCnt, /*format*/
    };
    size_t data_used = camera_metadata_type_size[4];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  } else if (runCase == WPE_PQ) {
    // TODO(MTK): need set case by case
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R1 << 8) | openId;
    int64_t  f0ParamCnt   = 1;
    int64_t  f0Setting    = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format     = MTK_HAL3PLUS_STREAM_FORMAT_NV21;

    int64_t  tuningStreamId   = Ids[1].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    //
    int64_t value[] = {
        2, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*yv12*/
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, /*physicCameraId:0, real time, streaming, undefined feature, isp tuning, serial number 0 */
        tuningParamCnt, /*format*/
    };
    size_t data_used = camera_metadata_type_size[2];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  } else if (runCase == FD_YUV_STREAM) {
    // TODO(MTK): need set case by case
    MY_LOGD("fd sid : %d", Ids[0].u8);
    int64_t  f0StreamId = Ids[0].u8;
    int64_t  f0Descriptor =  0x20000000 | (MTK_HAL3PLUS_STREAM_SOURCE_FD << 8) | openId;
    int64_t  f0ParamCnt  = 1;
    int64_t  f0Setting     = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format      = MTK_HAL3PLUS_STREAM_FORMAT_NV12;

    int64_t value[] = {
        1, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*yv12*/
    };
    size_t data_used = camera_metadata_type_size[3];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  } else if (runCase == MSNR_RAW_YUV) {
    // TODO(MTK): need set case by case
    int64_t  f0StreamId = Ids[0].u8;
    int64_t  f0Descriptor =  (MTK_HAL3PLUS_STREAM_SOURCE_FULL_RAW<<8) | openId;
    int64_t  f0ParamCnt  = 1;
    int64_t  f0Setting     = MTK_HAL3PLUS_STREAM_PARAMETER_FORMAT;
    int64_t  f0Format      = MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK;

    int64_t  tuningStreamId = Ids[1].u8;
    int64_t  tuningDescriptor =  (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt  = 0;

    int64_t value[] = {
        2, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, /*physic:0, real time, streaming, undefined feature, yuv, yuv0*/
        f0ParamCnt, /*param count*/
        f0Setting, /*setting format*/
        f0Format, /*RAW16*/
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, /*physicCameraId:0, real time, streaming, undefined feature, isp tuning, serial number 0 */
        tuningParamCnt, /*format*/
    };
    size_t data_used = camera_metadata_type_size[3];
    size_t cnt = (sizeof(value)/sizeof(value[0]));
    setMetadata(settings, MTK_HAL3PLUS_STREAM_SETTINGS, 1, data_used, cnt,
                reinterpret_cast<void *> (value));
  }

  mExtHandler->configure(&extOutputStreams, &maxBuffers,
                         bEnableTuningData, &settings);
  sleep(1);
  // native config
  if (bEnableNativeDevice) {
    std::vector<int32_t> SensorIdSet = {openId};
    std::vector<Stream> InputStreams;
    /**
     * Native Device Config:
     * For WPE_PQ device, the config input streams should include
     * one input image stream with format NV21 or YV12, and one
     * input warp map with format Blob, and one tuning stream.
     * There should be one or two output image.
     * The tuning stream do not need to config.
     * The format of output image stream is common image format.
     *
     * TO-DO: input warp map
     *
     * For MCNR device on isp6s, the config input streams should include
     * two input image streams with format YV12 (native device) or blob (HAL3-like),
     * and one tuning stream. The output image is the input of WPE_PQ device, the
     * output image format should be NV21 or YV12.
     *
     * For MCNR device on isp7, the config input streams should include
     * two input image streams with format packed10 and packed12 (native device)
     * or blob (HAL3-like), and one tuning stream.
     * The output image is the input of WPE_PQ device, the output image format
     * should be NV21 or YV12.
     */
      auto calculateSize = [] (
        const ::android::hardware::camera::device::V3_4::Stream& vec, uint32_t* size) {
        if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YV12){
          *size = vec.v3_2.width * vec.v3_2.height * 1.5;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_NV21){
          uint32_t alignWidth = ((vec.v3_2.width + 63) / 64 ) * 64;
          *size = alignWidth * vec.v3_2.height * 1.5;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_NV12){
          *size = vec.v3_2.width * vec.v3_2.height * 1.5;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK){
          uint32_t tmpStride = vec.v3_2.width * 10 / 8;
          uint32_t alignWidth = ((tmpStride + 79) / 80 ) * 80;
          *size = alignWidth * vec.v3_2.height * 1.5;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK){
          uint32_t tmpStride = vec.v3_2.width * 12 / 8;
          uint32_t alignStride = ((tmpStride + 95) / 96 ) * 96;
          *size = alignStride * vec.v3_2.height * 1.5;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_Y8){
          *size = vec.v3_2.width * vec.v3_2.height;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK){    // YUV422 10bit
          uint32_t tmpStride = vec.v3_2.width * 10 / 8;
          uint32_t alignWidth = ((tmpStride + 79) / 80 ) * 80;
          *size = alignWidth * vec.v3_2.height * 2;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK){
          int alignWidth = ((vec.v3_2.width + 63) / 64 ) * 64;
          *size = alignWidth * 1.25 * vec.v3_2.height;
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_WARP_2PLANE){
          *size = vec.v3_2.width * vec.v3_2.height * 4 * 2;
        } else {
          *size = vec.bufferSize;
        }
      };

      auto calculateStride = [] (
        const ::android::hardware::camera::device::V3_4::Stream& vec, std::vector<uint32_t>* stride) {
        MY_LOGD("calculateStride format %d", static_cast<uint32_t>(vec.v3_2.format));
        if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YV12){
          *stride = {vec.v3_2.width, vec.v3_2.width/2, vec.v3_2.width/2};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_NV21){
          uint32_t alignWidth = ((vec.v3_2.width + 63) / 64 ) * 64;
          *stride = {alignWidth, alignWidth};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_NV12){
          *stride = {vec.v3_2.width, vec.v3_2.width};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P010_PAK){
          uint32_t tmpStride = vec.v3_2.width * 10 / 8;
          uint32_t alignWidth = ((tmpStride + 79) / 80 ) * 80;
          *stride = {alignWidth, alignWidth};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_Y8){
          *stride = {vec.v3_2.width};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P210_PAK){    // YUV422 10bit
          uint32_t tmpStride = vec.v3_2.width * 10 / 8;
          uint32_t alignWidth = ((tmpStride + 79) / 80 ) * 80;
          *stride = {alignWidth, alignWidth};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_RAW10_PAK){
          int alignWidth = ((vec.v3_2.width + 63) / 64 ) * 64;
          *stride = {vec.v3_2.width * 10 / 8};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_YUV_P012_PAK){
          uint32_t tmpStride = vec.v3_2.width * 12 / 8;
          uint32_t alignStride = ((tmpStride + 95) / 96 ) * 96;
          MY_LOGD("P012 stride:%d, %d",tmpStride, alignStride);
          *stride = {alignStride, alignStride};
        } else if (static_cast<uint32_t>(vec.v3_2.format) == MTK_HAL3PLUS_STREAM_FORMAT_WARP_2PLANE){
          *stride = {vec.v3_2.width * 4};
        } else {
          *stride = {vec.v3_2.width, vec.v3_2.width, vec.v3_2.width};
        }
      };

    for (auto&& vec : nativeInputStreams) {

      Stream tmp;
      tmp.id = vec.v3_2.id;                       // stream ID <customize zone knowns>
      calculateSize(vec, &tmp.size);              // bufferSize, ex:3136320 (1920*1080 YV12) <customize zone calculate using size and format>
      calculateStride(vec, &tmp.stride);          // stride, ex:{1920, 960, 960} (1920*1080 YV12) <query from Hal3-like I/F>
      tmp.width = vec.v3_2.width;                 // width, ex:1920 <customize zone knowns>
      tmp.height = vec.v3_2.height;               // height, ex:1080 <customize zone knowns>
      tmp.format = (uint32_t)vec.v3_2.format;
      MY_LOGI("src fmt:%d, dst fmt:%d, stride:%d, size:%d", vec.v3_2.format, tmp.format, tmp.stride[0], tmp.size);
      tmp.transform = 0;
      tmp.batchsize = 1;
      InputStreams.push_back(tmp);
    }
    std::vector<Stream> OutputStreams;
    for (auto&& vec : nativeOutputStreams) {
      Stream tmp;
      tmp.id = vec.v3_2.id;
      calculateSize(vec, &tmp.size);
      calculateStride(vec, &tmp.stride);
      tmp.width = vec.v3_2.width;
      tmp.height = vec.v3_2.height;
      tmp.format = (uint32_t)vec.v3_2.format;
      MY_LOGI("src fmt:%d, dst fmt:%d, stride:%d, size:%d", vec.v3_2.format, tmp.format, tmp.stride[0], tmp.size);
      tmp.transform = 0;  // TODO(MTK): set transform.
      tmp.batchsize = 1;  // TODO(MTK): set batch size.
      OutputStreams.push_back(tmp);
    }

    //Add MCNR config metadata
    if (mExtHandler->getDeviceType() == "ISP_VIDEO")
    {
      auto MCNRsettings = settings;

      /**
       * MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES
       *
       * Configuring time bit mask to desdribe features to be enabled.
       */
      int64_t configFeature = MTK_NATIVEDEV_ISPVDO_AVAILABLE_FEATURES_YUVMCNR;   // mtk_camera_metadata_enum_nativedev_ispvdo_available_features
      size_t data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES),
                         1);
      setMeta(MCNRsettings,
              MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES,
              1,
              data_used,
              1,
              reinterpret_cast<void*> (&configFeature));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES(%d), %d",
              MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES, configFeature);

      /**
       * MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL
       *
       * This tag is used to specify controls.
       */
      int64_t featureControlNum = 3;
      int64_t featureControl[featureControlNum*2];
      featureControl[0] = MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_INLINE_WRAP_ENABLE;      // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
      featureControl[1] = 0;                                                                    // value of [0]
      featureControl[2] = MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_PQ_DIP_ENABLE;           // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
      featureControl[3] = 0;                                                                    // value of [2]
      featureControl[4] = MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_DOWNSCALE_LEVEL;         // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
      featureControl[5] = MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_2;                       // value of [4]
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL),
                         featureControlNum*2);
      featureControl[5] = ::property_get_int32("vendor.debug.hal3ut.dslevel", MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_2);
      MY_LOGD("ds_level:%d", featureControl[5]);
      setMeta(MCNRsettings,
              MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL,
              1,
              data_used,
              featureControlNum*2,
              reinterpret_cast<void*> (&featureControl));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL(%d), featureControlNum:%d",
              MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL, featureControlNum);

      /**
       * MTK_NATIVEDEV_ISPVDO_TUNING_HINT
       *
       * External tuning hint. This tag can be NULL, use MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT as default.
       */
      int64_t tuningNum = nativeInputStreams.size();               // equals to input stream size
      int64_t tuningHint[tuningNum*2];
      int tmp = 0;
      for (auto vec : nativeInputStreams) {
      tuningHint[tmp] = vec.v3_2.id;                                  // stream ID
      tuningHint[tmp+1] = MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT;   // mtk_camera_metadata_enum_nativedev_ispvdo_tuning_hint
      tmp+=2;
      }
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_TUNING_HINT),
                         tuningNum*2);
      setMeta(MCNRsettings,
              MTK_NATIVEDEV_ISPVDO_TUNING_HINT,
              1,
              data_used,
              tuningNum*2,
              reinterpret_cast<void*> (&tuningHint));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_TUNING_HINT(%d), tuningNum:%d",
              MTK_NATIVEDEV_ISPVDO_TUNING_HINT, tuningNum);

      /**
       * MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP
       *
       * This tag describes the relationship between Stream IDs and feature-based I/O.
       * Caller need to define the stream usage of each input stream for YUV_MCNR.
       */
      int64_t streamUsageMapNum = 1;
      if (runCase == MCNR_F0_F1) {
        streamUsageMapNum = 2;                                          // equals to number of input stream
      } else if(runCase == MCNR_F0_F1_ME) {
        streamUsageMapNum = 4;
      }

      int64_t streamUsageMap[streamUsageMapNum*2];
      streamUsageMap[0] = nativeInputStreams[0].v3_2.id;                      // stream ID of f0
      streamUsageMap[1] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F0 ;      // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
      if (runCase == MCNR_F0_F1) {
      streamUsageMap[2] = nativeInputStreams[1].v3_2.id;                      // stream ID of f1
      streamUsageMap[3] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1 ;      // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
      } else if(runCase == MCNR_F0_F1_ME) {
      streamUsageMap[2] = nativeInputStreams[1].v3_2.id;                      // stream ID of f1
      streamUsageMap[3] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1 ;      // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
      streamUsageMap[4] = nativeInputStreams[2].v3_2.id;                      // stream ID of ME
      streamUsageMap[5] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_ME ;      // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
      streamUsageMap[6] = nativeInputStreams[3].v3_2.id;                      // stream ID of ME
      streamUsageMap[7] = MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_TME ;      // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
      }
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP),
                         streamUsageMapNum*2);
      setMeta(MCNRsettings,
              MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP,
              1,
              data_used,
              streamUsageMapNum*2,
              reinterpret_cast<void*> (&streamUsageMap));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP(%d), f0:%d, f0 enum:%d",
              MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP, streamUsageMap[0], streamUsageMap[1]);


      camera_metadata *pConfigureParams
                       = reinterpret_cast<camera_metadata*> (MCNRsettings.data());
      mNativeHandler->configureNativeDevice(SensorIdSet, InputStreams,
                                            OutputStreams, pConfigureParams,
                                            mNativeCallbackHandler);
    } else {
      camera_metadata *pConfigureParams
                       = reinterpret_cast<camera_metadata*> (settings.data());
      mNativeHandler->configureNativeDevice(SensorIdSet, InputStreams,
                                            OutputStreams, pConfigureParams,
                                            mNativeCallbackHandler);
    }
  }
  return 0;
}

void Hal3Plus::allocateGraphicBuffer(
        ::android::hardware::camera::device::V3_4::Stream srcStream,
        StreamBuffer* dstBuffers,
        uint64_t bufferId) {
  auto width = srcStream.v3_2.width;
  auto height = srcStream.v3_2.height;
  auto usage = srcStream.v3_2.usage;
  auto format = srcStream.v3_2.format;
  hidl_handle buffer_handle;
  if (format == PixelFormat::IMPLEMENTATION_DEFINED) {
    format = PixelFormat::YCRCB_420_SP;
    MY_LOGD("format is YCRCB_420_SP");
  } else if (format == PixelFormat::YCBCR_420_888) {
    format = PixelFormat::YCRCB_420_SP;
    MY_LOGD("format is YCRCB_420_SP");
  } else if (format == PixelFormat::BLOB) {
    height = 1;
    width = srcStream.bufferSize;
    MY_LOGD("format is blob");
  } else {
    MY_LOGD("format unknown: (%d)", format);
  }
  IMapper::BufferDescriptorInfo descriptorInfo {
    .width = width,
    .height = height,
    .layerCount = 1,
    .format = (::android::hardware::graphics::common::V1_2::PixelFormat)format,
    .usage  = usage
  };
  BufferDescriptor descriptor;
  auto ret = mMapper->createDescriptor(descriptorInfo,
    [&descriptor](const auto& err, const auto& desc) {
       if (err != ::android::hardware::graphics::mapper::V4_0::Error::NONE) {
         MY_LOGE("IMapper::createDescriptor() failed ");
       }
    descriptor = desc;
  });
  ret = mAllocator->allocate(descriptor, 1u,
    [&] (auto err, uint32_t stride, const auto& buffers) {
      if (err != ::android::hardware::graphics::mapper::V4_0::Error::NONE) {
        MY_LOGE("IAllocate allocate buffer failed");
      }
      CAM_LOGD("stride(%u), width(%d), height(%d)", stride, width, height);
      buffer_handle = buffers[0];
  });
  *dstBuffers = {srcStream.v3_2.id,
                           bufferId,
                           buffer_handle,
                           BufferStatus::OK,
                           nullptr,
                           nullptr};
}

int Hal3Plus::allocateStreamBuffers() {
  auto size = extOutputStreams.size();
  MY_LOGI("extOutputStreams size:%lu", size);
  extOutputBuffers.resize(maxBuffers*size);
  ispTuningBuffers.resize(maxBuffers);
  nativeOutputBuffers.resize(maxBuffers);
  nativeInputBuffers.resize(maxBuffers);

  std::vector<data> Ids;
  for (auto vec : hal3reqs) {
    if (runCase == MCNR_F0_F1_ME && vec.reqOutput.id.size() == 4) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (runCase == MCNR_F0_F1 && vec.reqOutput.id.size() == 3) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (runCase == MCNR_F0 && vec.reqOutput.id.size() == 2) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (runCase == FD_YUV_STREAM && vec.reqOutput.id.size() == 1) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (runCase == WPE_PQ && vec.reqOutput.id.size() == 2) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (runCase == MSNR_RAW_YUV && vec.reqOutput.id.size() == 2) {
      MY_LOGD("find the MSNR_RAW_YUV");
      Ids = vec.reqOutput.id;
      break;
    }
  }
  uint64_t bufId = 1;
  for (auto num =0; num < Ids.size(); num++) {
    for (auto outInfo : allocateInfo) {
      if ((outInfo.streamId == (Ids[num]).u8) && num < (Ids.size()-1)) {
        ::android::hardware::hidl_vec<StreamBuffer> extOutputBuf;
        extOutputBuf.resize(maxBuffers);
        ::android::hardware::camera::device::V3_4::Stream stream3_4;
        for (auto out : extOutputStreams) {
          if (out.v3_2.id == (Ids[num]).u8) {
            stream3_4 = out;
            break;
          }
        }
        if (stream3_4.v3_2.format == PixelFormat::BLOB) {
          int blobSize = 0, planeNum = 0;
          for (auto&& onePlane: outInfo.planes) {
            auto id     = onePlane.planeId;
            auto stride = onePlane.stride;
            auto w      = onePlane.width;
            auto h      = onePlane.height;
            MY_LOGI("AllocateInfo: planId:%d, width:%d, height:%d, stride:%d", id, w, h, stride);
            if (planeNum == 0) {
              blobSize = stride * h;
            } else {
              blobSize = blobSize + stride * h / 2;
            }
            planeNum++;
          }
          MY_LOGI("AllocateInfo: streamID:%d, bufferSize:%d", outInfo.streamId, blobSize);
          if (blobSize == 0)
            MY_LOGF("BLOB Size from AllocateInfo is 0");

          stream3_4.v3_2.width  = blobSize;
          stream3_4.v3_2.height = 1;
          stream3_4.bufferSize  = blobSize;
        }
        for (int i=0; i < maxBuffers;  i++) {
          allocateGraphicBuffer(stream3_4,
                                         &extOutputBuffers[i],
                                         bufId);
          bufId++;
          extOutputBuf[i] = extOutputBuffers[i];
        }
        MY_LOGI("id:%d, w:%d, h:%d, size:%d", outInfo.streamId, stream3_4.v3_2.width, stream3_4.v3_2.height, stream3_4.bufferSize);
        extOutputBufMap.add(outInfo.streamId, extOutputBuf);
        break;
      }
      if ((outInfo.streamId == (Ids[num]).u8) && num == (Ids.size()-1)) {
         ::android::hardware::camera::device::V3_4::Stream stream3_4;
        for (auto out : extOutputStreams) {
          if (out.v3_2.id == (Ids[num]).u8) {
            stream3_4 = out;
            break;
          }
        }
        if (stream3_4.v3_2.format == PixelFormat::BLOB) {
          stream3_4.v3_2.width = outInfo.planes[0].width;
          stream3_4.v3_2.height = outInfo.planes[0].height;
          stream3_4.bufferSize   = (outInfo.planes[0].stride * outInfo.planes[0].height);
          stream3_4.v3_2.format = PixelFormat::BLOB;
        }
        for (int i=0; i < maxBuffers;  i++) {
          allocateGraphicBuffer(stream3_4,
                                         &ispTuningBuffers[i],
                                         bufId);
          bufId++;
        }
        MY_LOGI("tuning: id:%d, w:%d, h:%d, size:%d", outInfo.streamId, stream3_4.v3_2.width, stream3_4.v3_2.height, stream3_4.bufferSize);
        extOutputBufMap.add(outInfo.streamId, ispTuningBuffers);
        break;
      }
    }
  }
  if (bEnableNativeDevice) {
    for (auto out : nativeOutputStreams) {
      if (out.v3_2.format == static_cast<::android::hardware::graphics::common::V1_0::PixelFormat>(MTK_HAL3PLUS_STREAM_FORMAT_BLOB)) {
        out.v3_2.format = PixelFormat::BLOB;
      } else if (out.v3_2.format == static_cast<::android::hardware::graphics::common::V1_0::PixelFormat>(MTK_HAL3PLUS_STREAM_FORMAT_YV12)) {
        out.v3_2.format = PixelFormat::YV12;
      } else if (out.v3_2.format == static_cast<::android::hardware::graphics::common::V1_0::PixelFormat>(MTK_HAL3PLUS_STREAM_FORMAT_NV21)) {
        out.v3_2.format = PixelFormat::YCRCB_420_SP;
      }
      for (int i=0; i < maxBuffers;  i++) {
        allocateGraphicBuffer(out,
                            &nativeOutputBuffers[i],
                            bufId);
        bufId++;
        MY_LOGD("allocateGraphicBuffer nativeOutputStreams, size:[%d,%d]", out.v3_2.width, out.v3_2.height);
        auto const& buf = nativeOutputBuffers[i];
        auto handle = (void *) (buf.buffer.getNativeHandle());
        MY_LOGD("assign buffer handle:%p", handle);
      }
      nativeOutputBufMap.emplace(out.v3_2.id, nativeOutputBuffers);
    }
    //allocate warp map buffer (blob)
    if (mExtHandler->getDeviceType() == "WPE_PQ")
    {
      auto out = nativeInputStreams[1];
      out.v3_2.format = PixelFormat::BLOB;
      out.v3_2.width  = out.v3_2.width * out.v3_2.height * 4 * 2;
      out.v3_2.height = 1;
      out.bufferSize  = out.v3_2.width;

      MY_LOGD("Use blob format to allocate warp map buffer");
      for (int i=0; i < maxBuffers;  i++) {
        allocateGraphicBuffer(out,
                              &nativeInputBuffers[i],
                              bufId);
        bufId++;
        MY_LOGD("allocateGraphicBuffer nativeInputStreams (warp map), size:[%d,%d]", out.v3_2.width, out.v3_2.height);
        auto const& buf = nativeInputBuffers[i];
        auto handle = (void *) (buf.buffer.getNativeHandle());
        MY_LOGD("assign buffer handle:%p", handle);
      }
    }
  }
  return 0;
}

int Hal3Plus::queueRequest() {
  run("Hal3Plus::native");
  for (int i=0; i< maxBuffers; i++) {
    ::android::hardware::hidl_vec<StreamBuffer> tmp;
    int size = hal3reqs[targetNum].reqOutput.id.size();
    MY_LOGI("size:%d", size);
    tmp.resize(size);
    int bufNum = 0;
    for (auto id : hal3reqs[targetNum].reqOutput.id) {
      MY_LOGI("id:%d", id.u8);
      auto bufHandl = extOutputBufMap.valueFor(id.u8);
      tmp[bufNum++] = bufHandl[i];
    }
    totalAvailableBufMap.emplace(i, tmp);
  }
  // ext part
  StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                                   BufferStatus::ERROR,
                                   nullptr, nullptr};
  for (int i = 0; i < runTimes; i++) {
    if (bEnableNativeDevice) {
      std::unique_lock<std::mutex> _l(nativeLock);
      if (i > 4/*totalAvailableBufMap.empty()*/) {
        auto timeOut = std::chrono::system_clock::now() +
                       std::chrono::seconds(2);
        nativeCondition.wait_until(_l, timeOut);
      }
    } else {
      if (i > 4) {
        std::unique_lock<std::mutex> lk(extLock);
        extCondition.wait(lk);
        auto iter = extAvailableBufMap.begin();
        totalAvailableBufMap.emplace((iter->first+maxBuffers), iter->second);
        extAvailableBufMap.erase(iter);
      }
    }
    auto iter = totalAvailableBufMap.begin();
    if (iter ==totalAvailableBufMap.end()) {
      MY_LOGI("please check the flow");
    }

    // test ROI
    bool bTestROI = ::property_get_bool("vendor.debug.hal3ut.testROI.enable", 0);
    TestROIParam mTestROIParam = {
        .bTestROI = bTestROI,
        .runCase  = runCase,
        .Ids = hal3reqs[targetNum].reqOutput.id,
        .openId = openId};

    InFlightRequest* inflightReq = new InFlightRequest();
    mExtHandler->processCaptureRequest(iter->second, emptyInputBuffer,
                                       inflightReq, (i+1), bEnableTuningData,
                                       mTestROIParam);
    extAvailableBufMap.emplace((i+1), iter->second);
    MY_LOGI("queue yuv output buffer handle:%p, tuning buffer handle:%p",
            iter->second[0].buffer.getNativeHandle(),
            iter->second[1].buffer.getNativeHandle());
    totalAvailableBufMap.erase(iter);
    {
      std::unique_lock<std::mutex> l(extLock);
      extInflightMap.add((i+1), inflightReq);
      MY_LOGI("frame:%d, leftNum:%ld, frameNum:%ld, size:%zu", i,
              inflightReq->numBuffersLeft, inflightReq->frameNumber,
              extInflightMap.size());
     }
  }
  return 0;
}

void Hal3Plus::start() {
  // get characteristic
  getCharacteristics();
  // open
  open();
  // config
  config();
  // allocate buffer
  MY_LOGI("maxBuffers:%d", maxBuffers);
  allocateStreamBuffers();
  MY_LOGI("allocat buffers done");
  // queue request
  queueRequest();
  // flush
  // mExtHandler->flush();
  // close
  do {
    std::unique_lock<std::mutex> _lock(nativeLock);
    nativeCondition.wait(_lock);
    MY_LOGI("extInflightMap size:%zu", extInflightMap.size());
  } while (extInflightMap.size() != 0);
  MY_LOGD("close LHS");
  mExtHandler->close();

  sleep(5);   // 5 sec
  MY_LOGD("close RHS");
  mNativeHandler->close();
}

int Hal3Plus::flush() {
  auto ret =  mExtHandler->flush();
  if (ret) {
    return 1;
  } else {
    return 0;
  }
}

auto Hal3Plus::readyToRun() -> android::status_t {
  return android::OK;
}

auto Hal3Plus::requestExit() -> void {
  std::unique_lock<std::mutex> lk(nativeLock);
  nativeCondition.wait(lk);
  Thread::requestExit();
  join();
}

void Hal3Plus::dumpData(const hidl_handle& bufferHandle, int size, int requestNum, std::string oritation) {
   int ion_fd;
   auto err = gralloc_extra_query(bufferHandle,
                                   GRALLOC_EXTRA_GET_ION_FD,
                                   &ion_fd);
    if (ion_fd >=0 && err == 0) {
      MY_LOGI("get ION FD success, %d", ion_fd);
    } else {
       MY_LOGE("get ION fd fail");
    }
    char* va = nullptr;
    va = reinterpret_cast<char*> (
                      mmap(NULL, size, PROT_READ, MAP_SHARED, ion_fd, 0));
    if (va) {
      char dir[] = {"/data/vendor/camera_dump"};
      char filename[100];
      snprintf(filename,
               sizeof(filename),
               "%s/%s_output_%d_%d.yuv",
               dir,
               oritation.c_str(),
               size,
               requestNum);
      FILE* fd;
      if ((fd= fopen(filename, "w"))) {
        fwrite(va, size, 1, fd);
        fclose(fd);
      } else {
        MY_LOGE("cannot open the file:%s", filename);
      }
    }
    munmap(va, size);
}

auto Hal3Plus::threadLoop() -> bool {
  // wait the ext interface done
  MY_LOGI("%s", __FUNCTION__);
  std::unique_lock<std::mutex> _l(extLock);
  MY_LOGI("wait the condition");
  extCondition.wait(_l, [this](){
    return !receivedFrame.empty();
  });

    /**
     * Native Device Request:
     * For WPE_PQ device, the request I/O streams should include
     * one input image and one output image. It also need the warp
     * map data and tuning data (for HAL metadata).
     * TO-DO: warp map data
     *
     * For MCNR device, we need 3 input buffers including two image buffers
     * from HAL3-like and one tuning stream with YV12 format.
     * The output stream would be one image stream with the same size as f0.
     */

  if (bEnableNativeDevice && !receivedFrame.empty()) {
    auto frameNum = receivedFrame.front();
    // CameraMetadata appControl= extInflightMap[frameNum]->collectedResult;
    extInflightMap.removeItem(frameNum);
    ::android::hardware::hidl_vec<StreamBuffer> tmp;
    auto iter = extAvailableBufMap.find(frameNum);
    if (iter!= extAvailableBufMap.end()) {
      MY_LOGI("frameNum:%d, size:%zu, id(%d, %ld), id(%d, %ld)", frameNum,
              iter->second.size(), (iter->second)[0].streamId,
              (iter->second)[0].bufferId, (iter->second)[1].streamId,
              (iter->second)[1].bufferId);
      // input buffer part
      struct InputRequestSet inputRequestSet;
      struct InputBufferSet inputStream;
      struct InputRequest   inputReq;
      auto& input = iter->second;
      int  size  = input.size();    // include image streams and tuning stream
      if (runCase == FD_YUV_STREAM) size ++;
      // input yuv part
      for (int num =0 ; num < (size -1); num++) {
        struct NativeBuffer *iNativeBuf = new NativeBuffer;
        struct BufHandle* iBufHandle = new BufHandle;
        struct Buffer* inputBuffer = new Buffer;
        if (iNativeBuf && iBufHandle && inputBuffer) {
          native_handle* bufHandle =
            const_cast<native_handle*> (input[num].buffer.getNativeHandle());
          iNativeBuf->pBuffer = reinterpret_cast<void *> (bufHandle);           // buffer handle from HAL3-like ouput
          iNativeBuf->type    = GRAPHIC_HANDLE;
          iNativeBuf->offset  = 0;
          int strId = input[num].streamId;    // stream ID from HAL3-like
          for (auto vec : extOutputStreams) {
            if (vec.v3_2.id == strId) {
              iNativeBuf->bufferSize = vec.bufferSize;   // buffer size from HAL3-like (json file), cusomize zone should know
              break;
            }
          }
          // TODO(MTK): need read this info from gralloc.
          MY_LOGI("input buffer handle:%p, id:%d, bufferSize:%d, %d",
                  iNativeBuf->pBuffer, strId,
                  iNativeBuf->bufferSize,
                  nativeInputStreams[num].bufferSize);
          //add dump code
          if (bEnableDump) {
            dumpData(bufHandle,  iNativeBuf->bufferSize, frameNum, "left");
          }
          iBufHandle->planeBuf = {*iNativeBuf};
          inputBuffer->streamId = strId;
          inputBuffer->buffer     = iBufHandle;
          inputBuffer->pinfo      = nullptr;
          // the input buffers which output by the same sensor and same time, usually is raw buffer or yuv buffer
          inputStream.buffers.push_back(*inputBuffer);
        } else {
          MY_LOGI("alloc struct fail, please check");
        }
      }
      // assign tME buffer except the first request
      if (runCase == MCNR_F0_F1_ME && frameNum>1) {
        auto prev_iter = extAvailableBufMap.find(frameNum-1);
        auto& p_input = prev_iter->second;
        auto tME_ID = nativeInputStreams[3].v3_2.id;
        struct NativeBuffer *iNativeBuf = new NativeBuffer;
        struct BufHandle* iBufHandle = new BufHandle;
        struct Buffer* inputBuffer = new Buffer;
        if (iNativeBuf && iBufHandle && inputBuffer) {
          native_handle* bufHandle =
            const_cast<native_handle*> (p_input[size-1].buffer.getNativeHandle());
          iNativeBuf->pBuffer = reinterpret_cast<void *> (bufHandle);           // ME buffer handle from N-1 request
          iNativeBuf->type    = GRAPHIC_HANDLE;
          iNativeBuf->offset  = 0;
          int strId = tME_ID;
          for (auto vec : extOutputStreams) {
            if (vec.v3_2.id == p_input[size-1].streamId) {
              iNativeBuf->bufferSize = vec.bufferSize;   // buffer size from HAL3-like (json file), cusomize zone should know
              break;
            }
          }
          // TODO(MTK): need read this info from gralloc.
          MY_LOGI("ME(N-1) input buffer handle:%p, id:%d, bufferSize:%d, %d",
                  iNativeBuf->pBuffer, strId,
                  iNativeBuf->bufferSize,
                  nativeInputStreams[size-1].bufferSize);
          //add dump code
          if (bEnableDump) {
            dumpData(bufHandle,  iNativeBuf->bufferSize, frameNum, "left");
          }
          iBufHandle->planeBuf = {*iNativeBuf};
          inputBuffer->streamId = strId;
          inputBuffer->buffer     = iBufHandle;
          inputBuffer->pinfo      = nullptr;
          // the input buffers which output by the same sensor and same time, usually is raw buffer or yuv buffer
          inputStream.buffers.push_back(*inputBuffer);
        } else {
          MY_LOGI("alloc struct fail, please check");
          }
      }

      // tuningBuffer
      if (runCase != FD_YUV_STREAM) {
        auto& tuningBuf = input[size-1];         // assume the last stream is tuning stream
        struct NativeBuffer *iTNativeBuf = new NativeBuffer;
        struct BufHandle* iTBufHandle = new BufHandle;
        if (iTNativeBuf) {
          native_handle* bufHandle =
            const_cast<native_handle*> (tuningBuf.buffer.getNativeHandle());
          iTNativeBuf->pBuffer = reinterpret_cast<void *> (bufHandle);
          iTNativeBuf->type    = GRAPHIC_HANDLE;
          iTNativeBuf->offset  = 0;
          int strId = tuningBuf.streamId;
          for (auto vec : extOutputStreams) {
            if (vec.v3_2.id == strId) {
              iTNativeBuf->bufferSize = vec.bufferSize;
              break;
            }
          }
          MY_LOGI("tuning buffer handle:%p, strId:%d, size:%d, buffer id:%lu",
                  iTNativeBuf->pBuffer,
                  strId,
                  iTNativeBuf->bufferSize,
                  tuningBuf.bufferId);
          iTBufHandle->planeBuf  = {*iTNativeBuf};
          inputStream.tuningBuffer = iTBufHandle;
        } else {
          MY_LOGE("cannot alloc tuning buffer");
        }
      }

      //Add warp map
      if (mExtHandler->getDeviceType() == "WPE_PQ")
      {
        // get buffer handle
        auto& inputBuf = nativeInputBuffers[0];
        buffer_handle_t Buffer_h = inputBuf.buffer.getNativeHandle();
        int bufferSize;
        for (auto&& vec : nativeInputStreams) {
            if (vec.v3_2.id == inputBuf.streamId) {
              bufferSize = vec.bufferSize;            // buffer size from json file, cusomize zone should know
              break;
            }
        }
        if (bufferSize!=32){
          MY_LOGE("warp map bufferSize is expected to be 32, %d", bufferSize);
        }

        // write warp map into buffer
        NSCam::MapperHelper::getInstance().importBuffer(Buffer_h);
        void* data = nullptr;
        const IMapper::Rect region{0, 0, bufferSize, 1};
        int fence = -1;

        NSCam::MapperHelper::getInstance().lock(
            Buffer_h,
            (BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN), region,
            fence, data);
        if (data==nullptr) {
          MY_LOGE("lock image failed");
        }

        uint8_t value[32] = {0};
        int32_t shift[2]  = {0};
        int32_t step[2]   = {0};
        int32_t OutputW   = nativeInputStreams[0].v3_2.width;
        int32_t OutputH   = nativeInputStreams[0].v3_2.height;
        int32_t WarpMapW  = 2;
        int32_t WarpMapH  = 2;

        step[0] = OutputW/(WarpMapW-1);
        step[1] = OutputH/(WarpMapH-1);

        for(int j = 0; j < WarpMapH; j++) {
            for(int i = 0; i < WarpMapW; i++) {
                int32_t tmpX = (!shift[0] && !i) ?
                                0 : ((shift[0]+step[0] * i -1)* 32); //packed 32bit
                int32_t tmpY = (!shift[1] && !j) ?
                                0 : ((shift[1]+step[1] * j -1) * 32); //packed 32bit

                int k = (j * WarpMapW + i) * 4;
                value[k+0]  = tmpX;
                value[k+1]  = tmpX >> 8;
                value[k+2]  = tmpX >> 16;
                value[k+3]  = tmpX >> 24;
                value[k+16+0]  = tmpY;
                value[k+16+1]  = tmpY >> 8;
                value[k+16+2]  = tmpY >> 16;
                value[k+16+3]  = tmpY >> 24;
                MY_LOGD("warp map X value: k:%d, %d, %d, %d, %d", k, value[k+0], value[k+1], value[k+2], value[k+3]);
                MY_LOGD("warp map Y value: k:%d, %d, %d, %d, %d", k, value[k+16+0], value[k+16+1], value[k+16+2], value[k+16+3]);
            }
        }

        bool enableWarpMapCrop = ::property_get_int32("vendor.debug.hal3ut.wpe.enableWarpMapCrop", 0);
        MY_LOGD("enableWarpMapCrop: %d", enableWarpMapCrop);
        if (enableWarpMapCrop) {

          struct WPE_Parameter
          {
            unsigned int in_width;      // nativeInputStreams[0].v3_2.width
            unsigned int in_height;     // nativeInputStreams[0].v3_2.height
            unsigned int in_offset_x;   // 0
            unsigned int in_offset_y;   // 0
            unsigned int out_width;     // 720  // vendor.debug.hal3ut.wpe.cropw
            unsigned int out_height;    // 1280 // vendor.debug.hal3ut.wpe.croph
            unsigned int warp_map_width;  // 2
            unsigned int warp_map_height; // 2
          };

          WPE_Parameter wpe;
          wpe.in_width = nativeInputStreams[0].v3_2.width;
          wpe.in_height = nativeInputStreams[0].v3_2.height;
          wpe.in_offset_x = 0;
          wpe.in_offset_y = 0;
          wpe.out_width = ::property_get_int32("vendor.debug.hal3ut.wpe.outputw", 720);
          wpe.out_height = ::property_get_int32("vendor.debug.hal3ut.wpe.outputh", 1280);
          wpe.warp_map_width = 2;
          wpe.warp_map_height = 2;

          if ((wpe.in_offset_x + wpe.out_width) >  wpe.in_width) {
            MY_LOGF ("Crop area is over input image width");
          } else if ((wpe.in_offset_y + wpe.out_height) >  wpe.in_height) {
            MY_LOGF ("Crop area is over input image height");
          }

          float stepX = (float)(wpe.out_width) / (float)(wpe.warp_map_width - 1);
          float stepY = (float)(wpe.out_height) / (float)(wpe.warp_map_height - 1);
          MY_LOGD("StepX = %f, StepY = %f",stepX,stepY);

          float oy = wpe.in_offset_y;
          float ox = 0;
          for (int i = 0; i < wpe.warp_map_height; i ++) {
            ox = wpe.in_offset_x;
            for (int j = 0; j < wpe.warp_map_width; j ++) {
              if (ox >= (wpe.in_offset_x + wpe.out_width))
                ox = wpe.in_offset_x + wpe.out_width - 1.0f;
              if (oy >= (wpe.in_offset_y + wpe.out_height))
                oy = wpe.in_offset_y + wpe.out_height - 1.0f;

              int32_t tmpX = (int)(ox * 32.0f);
              int32_t tmpY = (int)(oy * 32.0f);

                    int k = (i * wpe.warp_map_width + j) * 4;
                    value[k+0]  = tmpX;
                    value[k+1]  = tmpX >> 8;
                    value[k+2]  = tmpX >> 16;
                    value[k+3]  = tmpX >> 24;
                    value[k+16+0]  = tmpY;
                    value[k+16+1]  = tmpY >> 8;
                    value[k+16+2]  = tmpY >> 16;
                    value[k+16+3]  = tmpY >> 24;
                    MY_LOGD("warp map with crop X value: k:%d, %d, %d, %d, %d", k, value[k+0], value[k+1], value[k+2], value[k+3]);
                    MY_LOGD("warp map with crop Y value: k:%d, %d, %d, %d, %d", k, value[k+16+0], value[k+16+1], value[k+16+2], value[k+16+3]);

              ox += stepX;
            }
            oy += stepY;
          }
        }

        uint8_t* addr = static_cast<uint8_t*>(data);
        int32_t writedSize = 0;
        int i = 0;
        while (writedSize < bufferSize) {
          *addr = value[i];
          addr++;
          writedSize++;
          i++;
        }

        struct NativeBuffer *iNativeBufWarp = new NativeBuffer;
        struct BufHandle* iBufHandle = new BufHandle;
        struct Buffer* inputBuffer = new Buffer;
        if (iNativeBufWarp && iBufHandle && inputBuffer) {
          iNativeBufWarp->pBuffer = (void *)Buffer_h;
          iNativeBufWarp->type    = GRAPHIC_HANDLE;
          iNativeBufWarp->offset  = 0;
          iNativeBufWarp->bufferSize = bufferSize;
          int strId = inputBuf.streamId;

          MY_LOGI("input warp map buffer handle:%p, id:%d, bufferSize:%d, buffer id:%d",
                  iNativeBufWarp->pBuffer, strId,
                  iNativeBufWarp->bufferSize, inputBuf.bufferId);
          iBufHandle->planeBuf = {*iNativeBufWarp};
          inputBuffer->streamId = strId;
          inputBuffer->buffer     = iBufHandle;
          inputBuffer->pinfo      = nullptr;
          // the input buffers which output by the same sensor and same time, usually is raw buffer or yuv buffer
          inputStream.buffers.push_back(*inputBuffer);
        }
      }

     // ptuningMeta
     CameraMetadata outputMeta;
     outputMeta.resize(settings.size());
     // Usually is the result metadata of mtk camera output
     inputStream.ptuningMeta =
                reinterpret_cast<camera_metadata*>(outputMeta.data());

     // vector of InputBufferSet. the multiple sensor output buffers
     inputReq.buffers = {inputStream};
     // vector of InputRequest. for preview: vector = 1, for multi-frame capture: vector >= 1
     inputRequestSet.in_data = {inputReq};

    // output buffer part
    // vector of OutputBuffer
    struct OutputBufferSet nativeOutSet;
    int count = 0;
    for (auto&& bufNap : nativeOutputBufMap) {
      struct NativeBuffer* oNativeBuf = new struct NativeBuffer;
      auto& outputBufs = bufNap.second;
      auto& outputBuf = outputBufs[frameNum%maxBuffers];

      if (oNativeBuf) {
        native_handle* bufHandle =
         const_cast<native_handle*> (outputBuf.buffer.getNativeHandle());
        oNativeBuf->pBuffer = reinterpret_cast<void *> (bufHandle);
        oNativeBuf->type    = GRAPHIC_HANDLE;
        oNativeBuf->offset  = 0;
        oNativeBuf->bufferSize = nativeOutputStreams[count].bufferSize;
      } else {
        MY_LOGE("alloca output native buffer failed");
      }
      MY_LOGI("output buffer handle:%p, size:%d, buffer id:%lu",
            oNativeBuf->pBuffer,
            oNativeBuf->bufferSize,
            outputBuf.bufferId);

      struct BufHandle* oBufHandle = new struct BufHandle;
      if (oBufHandle) {
        oBufHandle->planeBuf = {*oNativeBuf};
      } else {
        MY_LOGE("alloca output buffer handle failed");
      }

      struct Buffer* outputBuffer = new struct Buffer;
      if (outputBuffer) {
        outputBuffer->streamId = outputBuf.streamId;
        outputBuffer->buffer   = oBufHandle;
        outputBuffer->pinfo    = nullptr;
      } else {
        MY_LOGE("alloca output buffer failed");
      }

      struct OutputBuffer nativeOutputBuf;
      // the ouput image buffer
      nativeOutputBuf.buffer = *outputBuffer;
      // crop information and the priority is higher than crop metadata. Null means use crop metadata to do crop.
      nativeOutputBuf.crop = nullptr;

      nativeOutSet.buffers.push_back(nativeOutputBuf);

      count++;
    }

      if (runCase == FD_YUV_STREAM) {
        nativeOutSet.buffers = {};  // FD dosen't need output buffer
      }

    //Add MCNR request metadata
    auto MCNR_ReqSettings = settings;
    auto WPEPQ_ReqSettings = settings;

    if (mExtHandler->getDeviceType() == "ISP_VIDEO")
    {
      /**
       * MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES
       *
       * Configuring time bit mask to desdribe features to be enabled.
       */
      int64_t configFeature = MTK_NATIVEDEV_ISPVDO_AVAILABLE_FEATURES_YUVMCNR;   // mtk_camera_metadata_enum_nativedev_ispvdo_available_features
      size_t data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES),
                         1);
      setMeta(MCNR_ReqSettings,
              MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES,
              1,
              data_used,
              1,
              reinterpret_cast<void*> (&configFeature));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES(%d), %d",
              MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES, configFeature);

      /**
       * MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI
       *
       * NativeDevice supports crop function at output image. If not given, use the full source image size as ROI
       */
      int64_t destROINum = 1;
      int64_t destROI[destROINum*5];
      int64_t ROIPositionX = 0, ROIPositionY = 0;                 // ROI's position
      int64_t ROIArrayWidth = 4000, ROIArrayHeight = 3000;        // ROI's size
      destROI[0] = nativeInputStreams[0].v3_2.id;         // stream ID
      destROI[1] = ROIPositionX;
      destROI[2] = ROIPositionY;
      destROI[3] = ROIArrayWidth;
      destROI[4] = ROIArrayHeight;
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI),
                         destROINum*5);
      setMeta(MCNR_ReqSettings,
              MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI,
              1,
              data_used,
              destROINum*5,
              reinterpret_cast<void*> (&destROI));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI(%d), stream:%d, position:[%d,%d], size:[%d,%d]",
              MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI, destROI[0], destROI[1], destROI[2], destROI[3], destROI[4]);

      /**
       * MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION
       *
       * NativeDevice supports rotation (0 / 90 / 180 / 270) at the output image(s)
       */
      int64_t destOrientationNum = 1;
      int64_t destOrientation[destOrientationNum*2];
      destOrientation[0] = nativeInputStreams[0].v3_2.id;     // stream ID
      destOrientation[1] = 0;                                 // Orientation value
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION),
                         destOrientationNum*2);
      setMeta(MCNR_ReqSettings,
              MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION,
              1,
              data_used,
              destOrientationNum*2,
              reinterpret_cast<void*> (&destOrientation));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION(%d), stream:%d, position:%d",
              MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION, destOrientation[0], destOrientation[1]);

      /**
       * MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL
       *
       * This tag is used to specify controls.
       */
      int64_t featureControlNum = 2;
      int64_t featureControl[featureControlNum*2];
      featureControl[0] = MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_INLINE_WRAP_ENABLE;      // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
      featureControl[1] = 0;                                                                    // value of [0]
      featureControl[2] = MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_PQ_DIP_ENABLE;           // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
      featureControl[3] = 0;                                                                    // value of [2]
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL),
                         featureControlNum*2);
      setMeta(MCNR_ReqSettings,
              MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL,
              1,
              data_used,
              featureControlNum*2,
              reinterpret_cast<void*> (&featureControl));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL(%d), featureControlNum:%d",
              MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL, featureControlNum);

      /**
       * MTK_NATIVEDEV_ISPVDO_TUNING_HINT
       *
       * External tuning hint. This tag can be NULL, use MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT as default.
       */
      int64_t tuningNum = nativeInputStreams.size();               // equals to input stream size
      int64_t tuningHint[tuningNum*2];
      int tmp = 0;
      for (auto vec : nativeInputStreams) {
      tuningHint[tmp] = vec.v3_2.id;                                  // stream ID
      tuningHint[tmp+1] = MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT;   // mtk_camera_metadata_enum_nativedev_ispvdo_tuning_hint
      tmp+=2;
      }
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_ISPVDO_TUNING_HINT),
                         tuningNum*2);
      setMeta(MCNR_ReqSettings,
              MTK_NATIVEDEV_ISPVDO_TUNING_HINT,
              1,
              data_used,
              tuningNum*2,
              reinterpret_cast<void*> (&tuningHint));
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_TUNING_HINT(%d), tuningNum:%d",
              MTK_NATIVEDEV_ISPVDO_TUNING_HINT, tuningNum);
    } else if (mExtHandler->getDeviceType() == "WPE_PQ") {   // add WPE_PQ request meta
      /**
       * MTK_NATIVEDEV_WPEPQ_OUTPUT_SIZE
       *
       * Describe the output image size of WPE module. The output image size should smaller or equal to input image size.
       */
      int64_t outputSize[2];
      int64_t outputWidth = 1920, outputHeight = 1080;
      outputSize[0] = ::property_get_int32("vendor.debug.hal3ut.wpe.outputw", outputWidth);
      outputSize[1] = ::property_get_int32("vendor.debug.hal3ut.wpe.outputh", outputHeight);
      size_t data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_WPEPQ_OUTPUT_SIZE),
                         2);
      setMeta(WPEPQ_ReqSettings,
              MTK_NATIVEDEV_WPEPQ_OUTPUT_SIZE,
              1,
              data_used,
              2,
              reinterpret_cast<void*> (&outputSize));
      MY_LOGD("WPE_PQ meta: MTK_NATIVEDEV_WPEPQ_OUTPUT_SIZE(%d), [%d,%d]",
              MTK_NATIVEDEV_WPEPQ_OUTPUT_SIZE, outputSize[0], outputSize[1]);

      /**
       * MTK_NATIVEDEV_WPEPQ_FLIP
       *
       * Describe the output image need to flip or not of WPE_PQ device.
       */
      uint8_t flip = 0;     // 0:disable, 1:flip vertical, 2:flip horizontal
      flip = ::property_get_int32("vendor.debug.hal3ut.wpe.flip", 0);
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_WPEPQ_FLIP),
                         1);
      setMeta(WPEPQ_ReqSettings,
              MTK_NATIVEDEV_WPEPQ_FLIP,
              1,
              data_used,
              1,
              reinterpret_cast<void*> (&flip));
      MY_LOGD("WPE_PQ meta: MTK_NATIVEDEV_WPEPQ_FLIP(%d), %d",
              MTK_NATIVEDEV_WPEPQ_FLIP, flip);

      /**
       * MTK_NATIVEDEV_WPEPQ_ROTATE
       *
       * Describe the output image need to flip or not of WPE_PQ device
       */
      uint8_t rotate = MTK_NATIVEDEV_WPEPQ_ROTATE_NONE;     // value from mtk_camera_metadata_enum_nativedev_wpepq_rotate
      rotate = ::property_get_int32("vendor.debug.hal3ut.wpe.rotate", MTK_NATIVEDEV_WPEPQ_ROTATE_NONE);
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_WPEPQ_ROTATE),
                         1);
      setMeta(WPEPQ_ReqSettings,
              MTK_NATIVEDEV_WPEPQ_ROTATE,
              1,
              data_used,
              1,
              reinterpret_cast<void*> (&rotate));
      MY_LOGD("WPE_PQ meta: MTK_NATIVEDEV_WPEPQ_ROTATE(%d), %d",
              MTK_NATIVEDEV_WPEPQ_ROTATE, rotate);

      /**
       * MTK_NATIVEDEV_WPEPQ_CROP_INFO
       *
       * Describe the crop info of WPE_PQ device
       */
      int32_t crop[4];
      crop[0] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropx", 0);        // x position
      crop[1] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropy", 0);        // y position
      crop[2] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropw", 1920);     // width
      crop[3] = ::property_get_int32("vendor.debug.hal3ut.wpe.croph", 1080);     // height
      data_used = calculate_camera_metadata_entry_data_size(
                         get_camera_metadata_tag_type(MTK_NATIVEDEV_WPEPQ_CROP_INFO),
                         4);
      setMeta(WPEPQ_ReqSettings,
              MTK_NATIVEDEV_WPEPQ_CROP_INFO,
              1,
              data_used,
              4,
              reinterpret_cast<void*> (&crop));
      MY_LOGD("WPE_PQ meta: MTK_NATIVEDEV_WPEPQ_CROP_INFO(%d), position:[%d,%d], size:[%d,%d]",
              MTK_NATIVEDEV_WPEPQ_CROP_INFO, crop[0], crop[1], crop[2], crop[3]);
    }

     std::shared_ptr<ProcRequest> nativeRequest = std::make_shared<ProcRequest>();
     // request number
     nativeRequest->reqNumber = frameNum;

     if (mExtHandler->getDeviceType() == "ISP_VIDEO") {
     // the app control metadata and user could append or modify tag to do specific function.
        nativeRequest->pAppCtrl  =
                    reinterpret_cast<camera_metadata*>(MCNR_ReqSettings.data());
     } else if (mExtHandler->getDeviceType() == "WPE_PQ") {
        nativeRequest->pAppCtrl  =
                    reinterpret_cast<camera_metadata*>(WPEPQ_ReqSettings.data());
     } else {
        nativeRequest->pAppCtrl  =
                    reinterpret_cast<camera_metadata*>(settings.data());
     }
     // the description of input image buffer
     nativeRequest->Inputs = inputRequestSet;
     // the description of output image buffer
     nativeRequest->Outputs = nativeOutSet;

     MY_LOGD("Send request to native device, deviceType:%s, reqNumber:%d", (mExtHandler->getDeviceType().c_str()), frameNum);
     mNativeHandler->processCapture(nativeRequest);
     ProcVec.push_back(nativeRequest);

      nativeAvailableBufMap.emplace(frameNum, iter->second);
      receivedFrame.erase(receivedFrame.begin());
      if (inputStream.tuningBuffer) {
        auto iTBufHandle = inputStream.tuningBuffer;
        MY_LOGI("tuning buffer NativeBuffer size:%lu",
                (iTBufHandle->planeBuf).size());
        for (auto buf : iTBufHandle->planeBuf) {
          auto iTNativeBuf = buf;
          delete &iTNativeBuf;
        }
        delete iTBufHandle;
      }
      for (auto num = 0; num < inputStream.buffers.size(); num++) {
        struct BufHandle* iBufHandle = inputStream.buffers[num].buffer;
        MY_LOGI("input buffer NativeBuffer size:%lu, inputStream.buffers:%lu",
                (iBufHandle->planeBuf).size(),
                inputStream.buffers.size());
        for (auto buf : iBufHandle->planeBuf) {
          auto iNativeBuf  = buf;
          delete &iNativeBuf;
        }
        delete iBufHandle;
        MY_LOGI("delete BufHandle success");
        // delete &(inputStream.buffers[num]);
      }
      // if (oNativeBuf) {
      //   delete oNativeBuf;
      // }
      // if (oBufHandle) {
      //   delete oBufHandle;
      // }
      // if (outputBuffer) {
      //   delete outputBuffer;
      // }
    }
  }
  return true;
}

Hal3Plus::extInterface::extInterface(android::sp< Hal3Plus > pHal3)
:pHal3Plus(pHal3) {
  MY_LOGI("extInterface construct");
}

std::string Hal3Plus::extInterface::getDeviceType() {
  return pHal3Plus->deviceType;
}

void Hal3Plus::extInterface::open(
  ::android::sp<::android::hardware::camera::device::V3_6::ICameraDevice> pDev,
  android::sp<extCallback> cb) {
  ::android::sp<ICameraDeviceSession> session3_2;
  pDev->open(cb, [&session3_2](auto status, const auto &newSession) {
    MY_LOGI("device::open returns status:%d", (int)status);
    if (status == Status::OK)
      session3_2 = newSession;
  });
  auto castResult_3_6 =
    ::android::hardware::camera::device::V3_6::ICameraDeviceSession::castFrom
      (session3_2);
  session = castResult_3_6;
  {
    MY_LOGI("open hidl camera device session");
    mLibHandle = dlopen(camDevice3HidlLib, RTLD_NOW);
    if (mLibHandle == NULL) {
      MY_LOGE("dlopen fail: %s", dlerror());
    }
    CreateExtCameraDeviceSessionInstance_t* CreateExtCameraDeviceSessionInstance =
      (CreateExtCameraDeviceSessionInstance_t*)dlsym(mLibHandle, "CreateExtCameraDeviceSessionInstance");
    const char* dlsym_error = dlerror();
    if (CreateExtCameraDeviceSessionInstance == NULL) {
      MY_LOGE("Cannot load symbol: %s, %s", dlsym_error, dlerror());
    } else {
      mExtCameraDeviceSession = CreateExtCameraDeviceSessionInstance(pHal3Plus->intanceId);
    }
  }
}

void Hal3Plus::extInterface::updateMetadata(
    ::android::hardware::hidl_vec<uint8_t> src,
    uint32_t tag,
    size_t data_type,
    size_t data_cnt,
    void* data) {
  android::status_t res = android::OK;

  const camera_metadata_t* pMetadata =
      reinterpret_cast<const camera_metadata_t*> (src.data());
  const size_t entryCapacity =
      get_camera_metadata_entry_capacity(pMetadata) + 1;
  const size_t dataCapacity =
      get_camera_metadata_data_capacity(pMetadata) +
          calculate_camera_metadata_entry_data_size(
          get_camera_metadata_tag_type(tag), data_cnt);

  camera_metadata_t* vendor = nullptr;
  vendor = allocate_camera_metadata(entryCapacity, dataCapacity);
  if (vendor == nullptr) {
    MY_LOGE("allocate_camera_metadata fail");
  }

  if (append_camera_metadata(vendor, pMetadata) != android::OK) {
    MY_LOGE("append_camera_metadata fail");
  }

  camera_metadata_entry_t entry;
  res = find_camera_metadata_entry(vendor, tag, &entry);
  if (res == android::NAME_NOT_FOUND) {
    res = add_camera_metadata_entry(vendor, tag, &entry, data_cnt);
  } else if (res == android::OK) {
    res = update_camera_metadata_entry(vendor, entry.index,
                                       data, data_cnt, &entry);
  }
  if (res != android::OK) {
    MY_LOGE("updateMetadata fail");
  }

  settings.setToExternal(reinterpret_cast<uint8_t *> (vendor),
                         get_camera_metadata_size(vendor),
                         true);
}

void Hal3Plus::extInterface::setMetadata(
  ::android::hardware::hidl_vec<uint8_t> src,
  uint32_t tag,
  uint32_t entry_capacity,
  uint32_t data_capacity,
  int32_t cnt,
  void* value) {
  const camera_metadata_t* pMetadata =
                       reinterpret_cast<const camera_metadata_t*> (src.data());
  camera_metadata_t* vendor = nullptr;
  const size_t entryCapacity = (get_camera_metadata_entry_capacity(pMetadata) +
                                entry_capacity);
  const size_t dataCapacity = (get_camera_metadata_data_capacity(pMetadata)+
                               data_capacity);
  vendor = allocate_camera_metadata(entryCapacity, dataCapacity);
  size_t memory_needed = calculate_camera_metadata_size(entryCapacity,
                                                        dataCapacity);

  if (!validate_camera_metadata_structure(vendor, &memory_needed)) {
    if (0!= add_camera_metadata_entry(vendor, tag, value, cnt)) {
      MY_LOGE("add  failed");
    } else {
      MY_LOGD("add  success");
      int ret = append_camera_metadata(vendor, pMetadata);
      if (!ret) {
        MY_LOGI("append session parameter success");
      } else {
        MY_LOGI("append session parameter failed");
      }
      settings.setToExternal(reinterpret_cast<uint8_t *> (vendor),
                             get_camera_metadata_size(vendor),
                             true);
    }
  } else {
    MY_LOGE("FAIL");
  }
}

void Hal3Plus::extInterface::constructDefaultSetting(
  bool enableTuningData,
  ::android::hardware::hidl_vec<uint8_t>* defaultSetting) {
  RequestTemplate reqTemplate;
  if (enableTuningData) {
     if (pHal3Plus->deviceType == "ISP_VIDEO") {
        reqTemplate = RequestTemplate::PREVIEW;
    } else if (pHal3Plus->deviceType == "ISP_CAPTURE") {
      reqTemplate = RequestTemplate::STILL_CAPTURE;
    } else if (pHal3Plus->deviceType == "WPE_PQ") {
      reqTemplate = RequestTemplate::PREVIEW;
    } else {
      reqTemplate = RequestTemplate::PREVIEW;
    }
  } else {
    reqTemplate = RequestTemplate::PREVIEW;
  }
  if (!session) {
    MY_LOGE("session is nullptr, please check");
  } else {
  auto ret = session->constructDefaultRequestSettings(reqTemplate,
            [&](auto status, const auto& req) {
            if (status == Status::OK) {
              settings = req;
              MY_LOGI("sessionParams size:%zu", req.size());
            }
    });
    if (ret.isOk()) {
      MY_LOGI("get the default request settings");
    }
  }
  *defaultSetting = settings;
}

void Hal3Plus::extInterface::configure(
  ::android::hardware::hidl_vec
    <::android::hardware::camera::device::V3_4::Stream>* streams,
  int* maxBuf,
  bool enableTuningData,
  ::android::hardware::hidl_vec<uint8_t>* defaultSetting) {
  ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
  ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
#if 0
  RequestTemplate reqTemplate;
  if (enableTuningData) {
     if (pHal3Plus->deviceType == "ISP_VIDEO") {
        reqTemplate = RequestTemplate::PREVIEW;
     }
    if (pHal3Plus->deviceType == "ISP_CAPTURE") {
      reqTemplate = RequestTemplate::STILL_CAPTURE;
    }
  } else {
    reqTemplate = RequestTemplate::PREVIEW;
  }
  if (!session) {
    MY_LOGE("session is nullptr, please check");
  } else {
  auto ret = session->constructDefaultRequestSettings(reqTemplate,
            [&](auto status, const auto& req) {
            if (status == Status::OK) {
              settings = req;
              MY_LOGI("sessionParams size:%zu", req.size());
            }
    });
    if (ret.isOk()) {
      MY_LOGI("get the default request settings");
    }
  }
  if (enableTuningData) {
    // receive yv12 isp tuning data
    int value = 2;
    setMetadata(settings,
                MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE,
                1, 4, 1,
                reinterpret_cast<void*> (&value));
  }

  *defaultSetting = settings;
#endif
  settings = *defaultSetting;
  config3_4.sessionParams = settings;
  config3_4.streams = *streams;
  config3_4.operationMode =
            android::hardware::camera::device::V3_2::
            StreamConfigurationMode::NORMAL_MODE;
  MY_LOGI("settings size:%zu", settings.size());

  if (session != nullptr) {
    config3_5.v3_4 = config3_4;
    config3_5.streamConfigCounter = 0;  // TODO(MTK): add config count.
    auto ret = session->configureStreams_3_6(config3_5,
      [&] (Status s,
           ::android::hardware::camera::device::V3_6::HalStreamConfiguration
                  halConfig) {
        if (s == Status::OK) {
          MY_LOGI("configure stream success, stream size:%zu",
                  halConfig.streams.size());
        }
        halStreamConfig = halConfig;
    });
    if (ret.isOk()) {
      MY_LOGI("configure stream ok");
      mExtCameraDeviceSession->getConfigurationResults (streams->size(),
        [&] (int status, const camera_metadata* results,  std::map<int, camera_metadata *> streamResults) {
          if (status == ::android::OK) {
            for (auto str : streamResults) {
              MY_LOGI("stream id:%d", str.first);
              int strNum = 0;
              for (auto buf : *streams) {
                if (buf.v3_2.id == str.first) {
                  break;
                }
                strNum++;
              }
              camera_metadata_ro_entry enTry;
              auto ret = find_camera_metadata_ro_entry(str.second, MTK_HAL3PLUS_STREAM_ALLOCATION_INFOS, &enTry);
              MY_LOGI("entry count:%zu", enTry.count);
              if (ret) {
                MY_LOGI("cannot find MTK_HAL3PLUS_STREAM_ALLOCATION_INFOS. Please check");
              } else {
                struct oneStream oneStr;
                oneStr.streamId = str.first;
                for (auto num =0 ; num < enTry.count; num++) {
                  if (num%4 == 0) {
                    struct onePlane info;
                    info.planeId = enTry.data.i64[0];
                    info.width = enTry.data.i64[1];
                    info.height = enTry.data.i64[2];
                    info.stride = enTry.data.i64[3];
                    MY_LOGI("streamId:%d, planId:%d, width:%d, height:%d, stride:%d", oneStr.streamId, info.planeId, info.width, info.height, info.stride);
                   oneStr.planes.push_back(info);
                  }
                }
                pHal3Plus->allocateInfo.push_back(oneStr);
              }
          }
        }
      });
      if (pHal3Plus->allocateInfo.size() == 0) {
        MY_LOGF("cannot get allocateInfo");
      }
    } else {
      MY_LOGF("config failed!");
    }
    // TODO(MTK): get max buffer from HalStreamConfiguration.
    *maxBuf = 12;
  }
}

void Hal3Plus::extInterface::processCaptureRequest(
  ::android::hardware::hidl_vec<StreamBuffer> outputBuffer,
  StreamBuffer inputBuffer,
  InFlightRequest* inflightReq,
  uint32_t frameNumber,
  bool bEnableTuningData,
  TestROIParam mTestROIParam) {
  // uint32_t frameNumber = 1;
  Status status = Status::INTERNAL_ERROR;
  Return<void> ret;
  // CameraMetadata defaultSettings;
  if (bEnableTuningData && frameNumber == 1) {
    int value = 0;
    if (pHal3Plus->deviceType == "ISP_VIDEO") {
      value = 1;
    }
    if (pHal3Plus->deviceType == "WPE_PQ") {
      value = 1;
    }
    if (pHal3Plus->deviceType == "ISP_CAPTURE") {
      value = 1;
    }
    if (pHal3Plus->deviceType == "FD") {
      value = 1;
    }
    setMetadata(settings,
                MTK_CONTROL_CAPTURE_ISP_TUNING_REQUEST,
                1, 4, 1,
                reinterpret_cast<void*> (&value));
  }
  if (frameNumber == 1) {
  int32_t crop[] = { 0, 0, 4000, 3000 };
  size_t data_used = calculate_camera_metadata_entry_data_size(
                      get_camera_metadata_tag_type(ANDROID_SCALER_CROP_REGION),
                      sizeof(crop)/sizeof(crop[0]));
  setMetadata(settings,
              MTK_SCALER_CROP_APP_REGION,
              1,
              data_used,
              sizeof(crop)/sizeof(crop[0]),
              reinterpret_cast<void*> (crop));
  float ratio = 1.0;
  data_used = calculate_camera_metadata_entry_data_size(
              get_camera_metadata_tag_type(MTK_CONTROL_ZOOM_RATIO),
              sizeof(float));
  setMetadata(settings,
              MTK_CONTROL_ZOOM_RATIO,
              1,
              data_used,
              1,
              reinterpret_cast<void*> (&ratio));
  }

  auto const bTestROI = mTestROIParam.bTestROI;
  auto const runCase = mTestROIParam.runCase;
  auto const& Ids = mTestROIParam.Ids;
  auto const openId = mTestROIParam.openId;
  auto testROIpattern = ::property_get_int32("vendor.debug.hal3ut.testROI.pattern", 0);
  int64_t testROIX = ::property_get_int32("vendor.debug.hal3ut.testROI.x", 0);
  int64_t testROIY = ::property_get_int32("vendor.debug.hal3ut.testROI.y", 0);
  int64_t testROIW = ::property_get_int32("vendor.debug.hal3ut.testROI.w", 2104);
  int64_t testROIH = ::property_get_int32("vendor.debug.hal3ut.testROI.h", 1560);
  if (testROIpattern == 0) {
    // original pattern
    testROIX = testROIX + (frameNumber - 1) * 8;  // frameNumber starts from 1
    testROIY = testROIY + (frameNumber - 1) * 8;
    testROIW = testROIW - (frameNumber - 1) * 16;
    testROIH = testROIH - (frameNumber - 1) * 16;
  } else if (testROIpattern == 1) {
    // pattern 1
    testROIW = testROIW + (frameNumber - 1) * 10;
    testROIH = testROIH + (frameNumber - 1) * 10;
  } else {
    // pattern 2 (with 4 align)
    testROIW = testROIW + (frameNumber - 1) * 4;
    testROIH = testROIH + (frameNumber - 1) * 4;
  }


  MY_LOGD("frameNum=%d, parse testROI (%d,%d,%d,%d)",
          frameNumber, testROIX, testROIY, testROIW, testROIH);

  if (bTestROI && runCase == MCNR_F0_F1_ME) {
    // create streamSettings with ROI
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R1 << 8) | openId;
    int64_t  f0ParamCnt   = 4;
    int64_t  f0Param_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  f0Param_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  f0Param_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  f0Param_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  f0Value_ROIX = testROIX;
    int64_t  f0Value_ROIY = testROIY;
    int64_t  f0Value_ROIW = testROIW;
    int64_t  f0Value_ROIH = testROIH;

    int64_t  f1StreamId   = Ids[1].u8;
    int64_t  f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R2 << 8) | openId;
    int64_t  f1ParamCnt   = 4;
    int64_t  f1Param_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  f1Param_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  f1Param_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  f1Param_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  f1Value_ROIX = testROIX;
    int64_t  f1Value_ROIY = testROIY;
    int64_t  f1Value_ROIW = testROIW;
    int64_t  f1Value_ROIH = testROIH;

    int64_t  MEStreamId   = Ids[2].u8;
    int64_t  MEDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_ME << 8) | openId;
    int64_t  MEParamCnt   = 4;
    int64_t  MEParam_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  MEParam_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  MEParam_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  MEParam_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  MEValue_ROIX = testROIX;
    int64_t  MEValue_ROIY = testROIY;
    int64_t  MEValue_ROIW = testROIW;
    int64_t  MEValue_ROIH = testROIH;

    int64_t  tuningStreamId   = Ids[3].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    // hardcode: use setProperty to control p1 dma port
    bool useR3R4 = 0;
    useR3R4 = ::property_get_int32("vendor.debug.hal3ut.user3r4", 0);
    MY_LOGD("P1 DMA port use R3R4 :%d", useR3R4);
    if (useR3R4 == 1) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R3 << 8) | openId;
      f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R4 << 8) | openId;
    }

    int64_t value[] = {
        4, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, f0ParamCnt,
        f0Param_ROIX, f0Param_ROIY, f0Param_ROIW, f0Param_ROIH,
        f0Value_ROIX, f0Value_ROIY, f0Value_ROIW, f0Value_ROIH,
        f1StreamId, /* f1 stream id*/
        f1Descriptor, f1ParamCnt,
        f1Param_ROIX, f1Param_ROIY, f1Param_ROIW, f1Param_ROIH,
        f1Value_ROIX, f1Value_ROIY, f1Value_ROIW, f1Value_ROIH,
        MEStreamId, /* ME stream id*/
        MEDescriptor, MEParamCnt,
        MEParam_ROIX, MEParam_ROIY, MEParam_ROIW, MEParam_ROIH,
        MEValue_ROIX, MEValue_ROIY, MEValue_ROIW, MEValue_ROIH,
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, tuningParamCnt
    };
    size_t data_type = camera_metadata_type_size[3];  // int64_t
    size_t data_cnt = (sizeof(value)/sizeof(value[0]));
    updateMetadata(settings,
                   MTK_HAL3PLUS_STREAM_SETTINGS,
                   data_type,
                   data_cnt,
                   reinterpret_cast<void*> (value));
  } else if (bTestROI && runCase == MCNR_F0_F1) {
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R1 << 8) | openId;
    int64_t  f0ParamCnt   = 4;
    int64_t  f0Param_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  f0Param_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  f0Param_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  f0Param_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  f0Value_ROIX = testROIX;
    int64_t  f0Value_ROIY = testROIY;
    int64_t  f0Value_ROIW = testROIW;
    int64_t  f0Value_ROIH = testROIH;

    int64_t  f1StreamId   = Ids[1].u8;
    int64_t  f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R2 << 8) | openId;
    int64_t  f1ParamCnt   = 4;
    int64_t  f1Param_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  f1Param_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  f1Param_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  f1Param_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  f1Value_ROIX = testROIX;
    int64_t  f1Value_ROIY = testROIY;
    int64_t  f1Value_ROIW = testROIW;
    int64_t  f1Value_ROIH = testROIH;

    int64_t  tuningStreamId   = Ids[2].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    // hardcode: use setProperty to control p1 dma port
    bool useR3R4 = 0;
    useR3R4 = ::property_get_int32("vendor.debug.hal3ut.user3r4", 0);
    MY_LOGD("P1 DMA port use R3R4 :%d", useR3R4);
    if (useR3R4 == 1) {
      f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R3 << 8) | openId;
      f1Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_YUV_R4 << 8) | openId;
    }

    int64_t value[] = {
        3, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, f0ParamCnt,
        f0Param_ROIX, f0Param_ROIY, f0Param_ROIW, f0Param_ROIH,
        f0Value_ROIX, f0Value_ROIY, f0Value_ROIW, f0Value_ROIH,
        f1StreamId, /* f1 stream id*/
        f1Descriptor, f1ParamCnt,
        f1Param_ROIX, f1Param_ROIY, f1Param_ROIW, f1Param_ROIH,
        f1Value_ROIX, f1Value_ROIY, f1Value_ROIW, f1Value_ROIH,
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, tuningParamCnt
    };
    size_t data_type = camera_metadata_type_size[3];  // int64_t
    size_t data_cnt = (sizeof(value)/sizeof(value[0]));
    updateMetadata(settings,
                   MTK_HAL3PLUS_STREAM_SETTINGS,
                   data_type,
                   data_cnt,
                   reinterpret_cast<void*> (value));
  } else if (bTestROI && runCase == MCNR_F0) {
    int64_t  f0StreamId   = Ids[0].u8;
    int64_t  f0Descriptor = (MTK_HAL3PLUS_STREAM_SOURCE_DEPTH << 8) | openId;
    int64_t  f0ParamCnt   = 4;
    int64_t  f0Param_ROIX = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_X;
    int64_t  f0Param_ROIY = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_Y;
    int64_t  f0Param_ROIW = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_WIDTH;
    int64_t  f0Param_ROIH = MTK_HAL3PLUS_STREAM_PARAMETER_ROI_HEIGHT;
    int64_t  f0Value_ROIX = testROIX;
    int64_t  f0Value_ROIY = testROIY;
    int64_t  f0Value_ROIW = testROIW;
    int64_t  f0Value_ROIH = testROIH;

    int64_t  tuningStreamId   = Ids[1].u8;
    int64_t  tuningDescriptor = (MTK_HAL3PLUS_STREAM_SOURCE_TUNING_DATA << 8) | openId;
    int64_t  tuningParamCnt   = 0;

    int64_t value[] = {
        2, /* total configured streams*/
        f0StreamId, /* f0 stream id*/
        f0Descriptor, f0ParamCnt,
        f0Param_ROIX, f0Param_ROIY, f0Param_ROIW, f0Param_ROIH,
        f0Value_ROIX, f0Value_ROIY, f0Value_ROIW, f0Value_ROIH,
        tuningStreamId,  /*isp tuning stream Id*/
        tuningDescriptor, tuningParamCnt
    };
    size_t data_type = camera_metadata_type_size[3];  // int64_t
    size_t data_cnt = (sizeof(value)/sizeof(value[0]));
    updateMetadata(settings,
                   MTK_HAL3PLUS_STREAM_SETTINGS,
                   data_type,
                   data_cnt,
                   reinterpret_cast<void*> (value));
  }

  std::shared_ptr<ResultMetadataQueue> resultQueue;
  auto resultQueueRet =
    session->getCaptureResultMetadataQueue(
      [&resultQueue](const auto& descriptor) {
        resultQueue = std::make_shared<ResultMetadataQueue>(
          descriptor);
        if (!resultQueue->isValid() ||
          resultQueue->availableToWrite() <= 0) {
          MY_LOGE("%s: HAL returns empty result metadata fmq,"
                                " not use it", __func__);
          resultQueue = nullptr;
        }
      });

  InFlightRequest extInflightReq = {static_cast<ssize_t>(outputBuffer.size()),
                                    false,
                                    pHal3Plus->supportsPartialResults,
                                    pHal3Plus->partialResultCount,
                                    resultQueue,
                                    frameNumber};
  ::android::hardware::camera::device::V3_4::CaptureRequest request;
  request.v3_2.frameNumber = frameNumber;
  request.v3_2.fmqSettingsSize = 0;
  if (frameNumber == 1 || bTestROI) {
    // update metadata for each request when testing ROI
    request.v3_2.settings = settings;
  } else {
    request.v3_2.settings.setToExternal(nullptr, 0, true);
  }
  // TODO(MTK): set the buffer handle to nullptr.
  request.v3_2.inputBuffer = inputBuffer;
  request.v3_2.outputBuffers = outputBuffer;
  if (frameNumber > 12) {
    request.v3_2.inputBuffer.buffer = nullptr;
    for (auto &vec : request.v3_2.outputBuffers) {
      vec.buffer = nullptr;
    }
  }

  uint32_t numRequestProcessed = 0;
  hidl_vec<BufferCache> cachesToRemove;
  Return<void> returnStatus = session->processCaptureRequest_3_4(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
  });
  if (status == Status::OK) {
    MY_LOGI("%d request, %u frame processed success, leftNum:%zd",
            numRequestProcessed,
            frameNumber,
            extInflightReq.numBuffersLeft);
    std::unique_lock<std::mutex> _l(pHal3Plus->extLock);
    inflightReq->frameNumber = extInflightReq.frameNumber;
    inflightReq->numBuffersLeft = extInflightReq.numBuffersLeft;
    inflightReq->hasInputBuffer  = extInflightReq.hasInputBuffer;
    inflightReq->numPartialResults = extInflightReq.numPartialResults;
    inflightReq->usePartialResult   = extInflightReq.usePartialResult;
    inflightReq->haveResultMetadata = extInflightReq.haveResultMetadata;
  }
}

bool Hal3Plus::extInterface::flush() {
  Return<Status> returnStatus = session->flush();
  if (returnStatus.isOk()) {
    MY_LOGI("flush success");
    return true;
  } else {
    return false;
  }
}

void Hal3Plus::extInterface::close() {
  session->close();
}

Hal3Plus::extCallback::extCallback(android::sp<Hal3Plus> pHal3)
  : pHal3Plus(pHal3) {
  MY_LOGI("construct extCallback");
}
Return<void> Hal3Plus::extCallback::notify(
                                    const hidl_vec<NotifyMsg>& messages) {
  std::lock_guard<std::mutex> l(pHal3Plus->extLock);
    MY_LOGI("get notify");
    for (size_t i = 0; i < messages.size(); i++) {
        switch (messages[i].type) {
            case MsgType::ERROR:
                if (ErrorCode::ERROR_DEVICE ==
                               messages[i].msg.error.errorCode) {
                    MY_LOGE("%s: Camera reported serious device error",
                          __func__);
                } else {
                    ssize_t idx = pHal3Plus->extInflightMap.indexOfKey(
                            messages[i].msg.error.frameNumber);
                    if (::android::NAME_NOT_FOUND == idx) {
                        MY_LOGE("%s: Unexpected error frame number: %u",
                              __func__, messages[i].msg.error.frameNumber);
                        break;
                    }
                    InFlightRequest *r =
                                    pHal3Plus->extInflightMap.editValueAt(idx);
                    if (ErrorCode::ERROR_RESULT ==
                                   messages[i].msg.error.errorCode &&
                            messages[i].msg.error.errorStreamId != -1) {
                        if (r->haveResultMetadata) {
                            MY_LOGE("%s: must report physical camera result "
                                    " error before the final capture result!",
                                    __func__);
                        } else {
                            /*for (size_t j = 0; j < mStreams.size(); j++) {
                                if (mStreams[j].v3_2.id ==
                                    messages[i].msg.error.errorStreamId) {
                                    hidl_string physicalCameraId =
                                                mStreams[j].physicalCameraId;
                                    bool idExpected =
                                              r->expectedPhysicalResults.find(
                                                   physicalCameraId) !=
                                              r->expectedPhysicalResults.end();
                                    if (!idExpected) {
                                        MY_LOGE("%s: ERROR_RESULT's error"
                                                " stream's physicalCameraId "
                                                "%s must be expected",
                                                __func__,
                                                physicalCameraId.c_str());
                                    } else {
                                        r->expectedPhysicalResults.erase(
                                                             physicalCameraId);
                                    }
                                    break;
                                }
                            }*/
                        }
                    } else {
                        r->errorCodeValid = true;
                        r->errorCode = messages[i].msg.error.errorCode;
                        r->errorStreamId = messages[i].msg.error.errorStreamId;
                  }
                }
                break;
            case MsgType::SHUTTER:
            {
                MY_LOGI("get the shutter");
                ssize_t idx = pHal3Plus->extInflightMap.indexOfKey(
                                          messages[i].msg.shutter.frameNumber);
                if (::android::NAME_NOT_FOUND == idx) {
                    MY_LOGE("%s: Unexpected shutter frame number! received: %u",
                          __func__, messages[i].msg.shutter.frameNumber);
                    break;
                }
                InFlightRequest *r = pHal3Plus->extInflightMap.editValueAt(idx);
                r->shutterTimestamp = messages[i].msg.shutter.timestamp;
            }
                break;
            default:
                MY_LOGE("%s: Unsupported notify message %d", __func__,
                      messages[i].type);
                break;
        }
    }

    return Void();
}

Return<void> Hal3Plus::extCallback::processCaptureResult_3_4(
    const hidl_vec
        <::android::hardware::camera::device::V3_4::CaptureResult>& results) {
    if (!pHal3Plus) {
        return Void();
    }

    //bool notify = false;
    //std::unique_lock<std::mutex> l(pHal3Plus->extLock);
    //uint32_t frameNumber = results[0].v3_2.frameNumber;
    for (size_t i = 0 ; i < results.size(); i++) {
        bool notify = false;
        std::unique_lock<std::mutex> l(pHal3Plus->extLock);
        auto frameNumber = results[i].v3_2.frameNumber;

        notify = processCaptureResultLocked(results[i].v3_2,
                                            results[i].physicalCameraMetadata);
        MY_LOGD("results size:%d, i:%d, frameNumber1:%d, notify:%d",results.size(), i, frameNumber, notify);
        if (notify) {
          MY_LOGI("frameNumber:%d", frameNumber);
          pHal3Plus->receivedFrame.push_back(frameNumber);
        }
        l.unlock();
        if (notify) {
            pHal3Plus->extCondition.notify_all();
        }
    }
    return Void();
}

bool Hal3Plus::extCallback::processCaptureResultLocked(
        const CaptureResult& results,
        hidl_vec<PhysicalCameraMetadata> physicalCameraMetadata) {
    bool notify = false;
    uint32_t frameNumber = results.frameNumber;
    if ((results.result.size() == 0) &&
            (results.outputBuffers.size() == 0) &&
            (results.inputBuffer.buffer == nullptr) &&
            (results.fmqResultSize == 0)) {
        MY_LOGE("%s: No result data provided by HAL"
                " for frame %d result count: %d",
                __func__, frameNumber, (int) results.fmqResultSize);
        return notify;
    }

    ssize_t idx = pHal3Plus->extInflightMap.indexOfKey(frameNumber);
    if (::android::NAME_NOT_FOUND == idx) {
        MY_LOGE("%s: Unexpected frame number! received: %u",
                __func__, frameNumber);
        return notify;
    }

    bool isPartialResult = false;
    bool hasInputBufferInRequest = false;
    InFlightRequest *request = pHal3Plus->extInflightMap.editValueAt(idx);
    MY_LOGI("process capture result, frameNumber:%d "
            ", usePartialResult:%d, result size:%zu, "
            " fmqResultSize:%lu",
            frameNumber,
            request->usePartialResult,
            results.result.size(),
            results.fmqResultSize);
    ::android::hardware::camera::device::V3_2::CameraMetadata resultMetadata;
    size_t resultSize = 0;
    if (results.fmqResultSize > 0) {
        resultMetadata.resize(results.fmqResultSize);
        if (request->resultQueue == nullptr) {
            return notify;
        }
        if (!request->resultQueue->read(resultMetadata.data(),
                    results.fmqResultSize)) {
            MY_LOGE("%s: Frame %d: Cannot read camera metadata from fmq,"
                    "size = %" PRIu64, __func__, frameNumber,
                    results.fmqResultSize);
            return notify;
        }

        if (physicalCameraMetadata.size() !=
            request->expectedPhysicalResults.size()) {
            MY_LOGE("%s: Frame %d: Returned physical metadata count %zu "
                    "must be equal to expected count %zu", __func__,
                    frameNumber,
                    physicalCameraMetadata.size(),
                    request->expectedPhysicalResults.size());
            return notify;
        }
        std::vector<::android::hardware::camera::device::V3_2::CameraMetadata>
                                                            physResultMetadata;
        physResultMetadata.resize(physicalCameraMetadata.size());
        for (size_t i = 0; i < physicalCameraMetadata.size(); i++) {
            physResultMetadata[i].resize(
                                  physicalCameraMetadata[i].fmqMetadataSize);
            if (!request->resultQueue->read(physResultMetadata[i].data(),
                    physicalCameraMetadata[i].fmqMetadataSize)) {
                MY_LOGE("%s: Frame %d: "
                        " Cannot read physical camera metadata from fmq,"
                        "size = %" PRIu64, __func__, frameNumber,
                        physicalCameraMetadata[i].fmqMetadataSize);
                return notify;
            }
        }
        resultSize = resultMetadata.size();
    } else if (results.result.size() > 0) {
        resultMetadata.setToExternal(const_cast<uint8_t *>(
                    results.result.data()), results.result.size());
        resultSize = resultMetadata.size();
    }

    if (!request->usePartialResult && (resultSize > 0) &&
            (results.partialResult != 1)) {
        MY_LOGE("%s: Result is malformed for frame %d: partial_result %u "
                "must be 1  if partial result is not supported", __func__,
                frameNumber, results.partialResult);
        return notify;
    }

    if (results.partialResult != 0) {
        request->partialResultCount = results.partialResult;
    }

    // Check if this result carries only partial metadata
    if (request->usePartialResult && (resultSize > 0)) {
        if ((results.partialResult > request->numPartialResults) ||
                (results.partialResult < 1)) {
            MY_LOGE("%s: Result is malformed for frame %u: partial_result %u"
                    " must be  in the range of [1, %d] when metadata is "
                    "included in the result", __func__, frameNumber,
                    results.partialResult, request->numPartialResults);
            return notify;
        }
        /*request->collectedResult.append(
                reinterpret_cast<const camera_metadata_t*>(
                    resultMetadata.data()));*/
       MY_LOGI("usePartialResult:%d, resultSize:%zu", request->usePartialResult,
               resultSize);
       request->collectedResult.resize(
                                resultSize+request->collectedResult.size());
       append_camera_metadata(
          reinterpret_cast<camera_metadata_t*>(request->collectedResult.data()),
          reinterpret_cast<const camera_metadata_t*>(resultMetadata.data()));

        isPartialResult =
            (results.partialResult < request->numPartialResults);
    } else if (resultSize > 0) {
      MY_LOGI("usePartialResult is not , resultSize:%zu", resultSize);
      request->collectedResult.resize(
                               resultSize+request->collectedResult.size());
      append_camera_metadata(
          reinterpret_cast<camera_metadata_t*>(request->collectedResult.data()),
          reinterpret_cast<const camera_metadata_t*>(resultMetadata.data()));
      isPartialResult = false;
    }

    hasInputBufferInRequest = request->hasInputBuffer;

    if ((resultSize > 0) && !isPartialResult) {
        if (request->haveResultMetadata) {
            MY_LOGE("%s: Called multiple times with metadata for frame %d",
                    __func__, frameNumber);
            return notify;
        }
        request->haveResultMetadata = true;
    }

    uint32_t numBuffersReturned = results.outputBuffers.size();
    if (results.inputBuffer.buffer != nullptr) {
        if (hasInputBufferInRequest) {
            numBuffersReturned += 1;
        } else {
            MY_LOGW("%s: Input buffer should be NULL if there is no input"
                    " buffer sent in the request", __func__);
        }
    }
    MY_LOGI("numBuffersLeft:%zu, numBuffersReturned:%d, idx:%zd, size:%zu",
            request->numBuffersLeft,
            numBuffersReturned,
            idx,
            results.outputBuffers.size());
    request->numBuffersLeft -= numBuffersReturned;
    if (request->numBuffersLeft < 0) {
        MY_LOGE("%s: Too many buffers returned for frame %d,"
                " numBuffersReturned:%d",
                __func__,
                frameNumber,
                numBuffersReturned);
        return notify;
    }

    if (/*results.outputBuffers.data() && */ request->numBuffersLeft  == 0) {
    // TODO(MTK): add judgement.
    // request->resultOutputBuffers.appendArray(results.outputBuffers.data(),
    //        results.outputBuffers.size());
      /*for (auto iter = pHal3Plus->bufferStatus.begin();
             iter != pHal3Plus->bufferStatus.end();
             iter++) {
        if (frameNumber == iter->first)
          iter->second = 0;
      }*/
    } else {
        MY_LOGE("the returned buffer is nullptr");
    }
    // If shutter event is received notify the pending threads.
    MY_LOGI("resultOutputBuffers append");
    if (request->shutterTimestamp != 0 && request->numBuffersLeft == 0) {
        if (results.outputBuffers.size() == 0) {
          MY_LOGI("output buffer returned successfully, skip this one");
        } else {
          notify = true;
        }
    }
    /*
    // error result, maybe flush, need check
    /*if (request->numBuffersLeft == 0 && resultSize == 0) {
      notify = true;
    }*/

    return notify;
}

Return<void> Hal3Plus::extCallback::requestStreamBuffers(
        const hidl_vec< BufferRequest>& bufReqs,
        ::android::hardware::camera::device::V3_5::ICameraDeviceCallback::
        requestStreamBuffers_cb _hidl_cb) {
    hidl_vec<StreamBufferRet> bufRets;
    std::unique_lock<std::mutex> l(pHal3Plus->extLock);

    if (!mUseHalBufManager) {
        MY_LOGE("%s: Camera does not support HAL buffer management",
                 __FUNCTION__);
        _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
        return Void();
    }

    if (bufReqs.size() > mStreams.size()) {
        MY_LOGE("%s: illegal buffer request: too many requests!", __FUNCTION__);
        _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
        return Void();
    }

    std::vector<int32_t> indexes(bufReqs.size());
    for (size_t i = 0; i < bufReqs.size(); i++) {
        bool found = false;
        for (size_t idx = 0; idx < mStreams.size(); idx++) {
            if (bufReqs[i].streamId == mStreams[idx].v3_2.id) {
                found = true;
                indexes[i] = idx;
                break;
            }
        }
        if (!found) {
            MY_LOGE("%s: illegal buffer request: unknown streamId %d!",
                    __FUNCTION__, bufReqs[i].streamId);
            _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
            return Void();
        }
    }

    bool allStreamOk = true;
    bool atLeastOneStreamOk = false;
    bufRets.resize(bufReqs.size());
    for (size_t i = 0; i < bufReqs.size(); i++) {
        int32_t idx = indexes[i];
        const auto& stream = mStreams[idx];
        const auto& halStream = mHalStreams[idx];
        const ::android::hardware::camera::device::V3_5::BufferRequest& bufReq
              = bufReqs[i];
        if ((mOutstandingBufferIds[idx].size() + bufReq.numBuffersRequested)
             > halStream.maxBuffers) {
            bufRets[i].streamId = stream.v3_2.id;
            bufRets[i].val.error(StreamBufferRequestError::MAX_BUFFER_EXCEEDED);
            allStreamOk = false;
            continue;
        }

        hidl_vec<StreamBuffer> tmpRetBuffers(bufReq.numBuffersRequested);
        atLeastOneStreamOk = true;
        bufRets[i].streamId = stream.v3_2.id;
        bufRets[i].val.buffers(std::move(tmpRetBuffers));
    }

    if (allStreamOk) {
        _hidl_cb(BufferRequestStatus::OK, bufRets);
    } else if (atLeastOneStreamOk) {
        _hidl_cb(BufferRequestStatus::FAILED_PARTIAL, bufRets);
    } else {
        _hidl_cb(BufferRequestStatus::FAILED_UNKNOWN, bufRets);
    }

    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }
    return Void();
}

Return<void> Hal3Plus::extCallback::returnStreamBuffers(
        const hidl_vec<StreamBuffer>& buffers) {
    if (!mUseHalBufManager) {
        MY_LOGE("%s: Camera does not support HAL buffer management",
                __FUNCTION__);
    }

    std::unique_lock<std::mutex> l(pHal3Plus-> extLock);
    for (const auto& buf : buffers) {
        bool found = false;
        for (size_t idx = 0; idx < mOutstandingBufferIds.size(); idx++) {
            if (mStreams[idx].v3_2.id == buf.streamId &&
                    mOutstandingBufferIds[idx].count(buf.bufferId) == 1) {
                mOutstandingBufferIds[idx].erase(buf.bufferId);
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        MY_LOGE("%s: unknown buffer ID %" PRIu64, __FUNCTION__, buf.bufferId);
    }
    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }
    return Void();
}

Return<void> Hal3Plus::extCallback::processCaptureResult(
        const hidl_vec<CaptureResult>& results) {
    if (!pHal3Plus) {
        return Void();
    }

    bool notify = false;

    std::unique_lock<std::mutex> l(pHal3Plus->extLock);
    ::android::hardware::hidl_vec<PhysicalCameraMetadata> noPhysMetadata;
    for (size_t i = 0 ; i < results.size(); i++) {
        notify = processCaptureResultLocked(results[i], noPhysMetadata);
    }

    l.unlock();
    if (notify) {
        pHal3Plus->extCondition.notify_one();
    }

    return Void();
}

bool Hal3Plus::extCallback::hasOutstandingBuffersLocked() {
    if (!mUseHalBufManager) {
        return false;
    }
    for (const auto& outstandingBuffers : mOutstandingBufferIds) {
        if (!outstandingBuffers.empty()) {
            return true;
        }
    }
    return false;
}

Hal3Plus::nativeInterface::nativeInterface(android::sp< Hal3Plus > pHal3)
:pHal3Plus(pHal3) {
  MY_LOGI("nativeInterface construct");
}

void Hal3Plus::nativeInterface::createNativeDevice() {
  NSCam::v3::NativeDev::NativeDevType devType;
  if (pHal3Plus->deviceType == "ISP_VIDEO") {
    devType = NSCam::v3::NativeDev::NativeDevType::ISP_VIDEO;
  }
  if (pHal3Plus->deviceType == "ISP_CAPTURE") {
    devType = NSCam::v3::NativeDev::NativeDevType::ISP_CAPTURE;
  }
  if (pHal3Plus->deviceType == "FD") {
    devType = NSCam::v3::NativeDev::NativeDevType::FD;
  }
  if (pHal3Plus->deviceType == "WPE_PQ") {
    devType = NSCam::v3::NativeDev::NativeDevType::WPE;
  }

  MY_LOGI("devType:%d", devType);
  pDevice = NativeModule::createDevice(devType);
}

void Hal3Plus::nativeInterface::configureNativeDevice(
                             std::vector<int32_t> SensorIdSet,
                             std::vector<Stream> InputStreams,
                             std::vector<Stream> OutputStreams,
                             camera_metadata *pConfigureParams,
                             std::shared_ptr<nativeCallback> pDeviceCallback) {
  struct StreamConfiguration ispConfig;
  ispConfig.SensorIdSet = SensorIdSet;
  ispConfig.InputStreams = InputStreams;
  ispConfig.OutputStreams = OutputStreams;
  ispConfig.pConfigureParams = pConfigureParams;
  MY_LOGI("%s++", __FUNCTION__);
  pDevice->configure(ispConfig, pDeviceCallback);
}

void Hal3Plus::nativeInterface::processCapture(
                                std::shared_ptr<ProcRequest> request) {
  std::vector<std::shared_ptr<ProcRequest>> requests = {request};
  pDevice->processRequest(requests);
}

void Hal3Plus::nativeInterface::close() {
  auto res = NativeModule::closeDevice(pDevice);
  if (res == 0) {
    MY_LOGI("Native module close success");
  }
}

Hal3Plus::nativeCallback::nativeCallback(android::sp<Hal3Plus> pHal3)
  :pHal3Plus(pHal3) {
  MY_LOGI("construct nativeCallback");
}
int32_t Hal3Plus::nativeCallback::processPartial(
                                  int32_t const& status,
                                  Result const& result,
                                  std::vector<BufHandle*> const& completedBuf) {
  auto reqNum = result.reqNumber;

  std::unique_lock<std::mutex> lk(pHal3Plus->nativeLock);
  auto iter = pHal3Plus->nativeAvailableBufMap.find(reqNum);
  if (iter != pHal3Plus->nativeAvailableBufMap.end()) {
    auto bufMap = iter->second;
    auto bufMapSize = bufMap.size();
    MY_LOGD("get partial result, reqNumber:%d, status:%d, bufMapSize:%d, returnBufSize:%d", reqNum, status, bufMapSize, completedBuf.size());
    for(auto&& returnBufHandle : completedBuf){
      auto& nativeBuf = returnBufHandle->planeBuf;
      auto& bufHand = nativeBuf[0].pBuffer;
      for(auto&& buf : bufMap) {

        if (buf.buffer.getNativeHandle() == bufHand){
          MY_LOGD("obtain buffer, reqNumber:%d, streamID:%d", reqNum, buf.streamId);
          break;
        }
      }
    }
    if(bufMapSize == completedBuf.size()){
      MY_LOGD("all buffers return in partial callback for request %d", reqNum);
    }
  }

  return 0;
}


int32_t Hal3Plus::nativeCallback::processCompleted(int32_t const& status,
                                                   Result const& result) {
  MY_LOGI("native process completed");
  std::unique_lock<std::mutex> lk(pHal3Plus->nativeLock);
  auto iter = pHal3Plus->nativeAvailableBufMap.find(result.reqNumber);
  if (iter != pHal3Plus->nativeAvailableBufMap.end()) {
    // add dump
    int ion_fd;
    int bufNum = result.reqNumber%12;
    auto outputBuf = pHal3Plus->nativeOutputBuffers[bufNum];
    auto strId = outputBuf.streamId;
    int size = 0;
    for (auto vec : pHal3Plus->nativeOutputStreams) {
      if (vec.v3_2.id == strId) {
        auto w = vec.v3_2.width;
        auto h = vec.v3_2.height;
        if (vec.v3_2.format == PixelFormat::RAW16 ||
            vec.v3_2.format == PixelFormat::YCBCR_422_I) {
          size = w*h*2;
          break;
        }
        if (vec.v3_2.format != PixelFormat::BLOB &&
            vec.v3_2.format != PixelFormat::RAW16) {
          size = w*h*1.5;
          break;
        }
        if (vec.v3_2.format != PixelFormat::BLOB) {
          size = vec.bufferSize;
          break;
        }
      }
    }
    auto err = gralloc_extra_query(outputBuf.buffer.getNativeHandle(),
                                   GRALLOC_EXTRA_GET_ION_FD,
                                   &ion_fd);
    if (ion_fd >=0) {
      MY_LOGI("get ION FD success, %d, size %d", ion_fd, size);
    } else {
       MY_LOGE("get ION fd fail");
    }
    char* va = nullptr;
    va = reinterpret_cast<char*> (
                      mmap(NULL, size, PROT_READ, MAP_SHARED, ion_fd, 0));
    if (va) {
      char dir[] = {"/data/vendor/camera_dump"};
      char filename[100];
      snprintf(filename,
               sizeof(filename),
               "%s/output_%d.yuv",
               dir,
               result.reqNumber);
      FILE* fd;
      if ((fd= fopen(filename, "w"))) {
        size_t err = 2;
        err = fwrite(va, size, 1, fd);
        MY_LOGD("fwrite req(%d) result: %zu", result.reqNumber, err);
        fclose(fd);
      } else {
        MY_LOGE("cannot open the file:%s", filename);
      }
    }
    munmap(va, size);
    auto input = (iter->second)[0];
    for (auto vec : pHal3Plus->extOutputStreams) {
      if (vec.v3_2.id == strId) {
        auto w = vec.v3_2.width;
        auto h = vec.v3_2.height;
        if (vec.v3_2.format == PixelFormat::RAW16 ||
            vec.v3_2.format == PixelFormat::YCBCR_422_I) {
          size = w*h*2;
          break;
        }
        if (vec.v3_2.format != PixelFormat::BLOB &&
            vec.v3_2.format != PixelFormat::RAW16) {
          size = w*h*1.5;
          break;
        }
        if (vec.v3_2.format != PixelFormat::BLOB) {
          size = vec.bufferSize;
          break;
        }
      }
    }
    err = gralloc_extra_query(input.buffer.getNativeHandle(),
                              GRALLOC_EXTRA_GET_ION_FD,
                              &ion_fd);
    if (ion_fd >=0) {
      MY_LOGI("get input ION FD success, %d, size %d", ion_fd, size);
    } else {
       MY_LOGE("get ION fd fail");
    }
    va = nullptr;
    va = reinterpret_cast<char*> (
                        mmap(NULL, size, PROT_READ, MAP_SHARED, ion_fd, 0));
    if (va) {
      char dir[] = {"/data/vendor/camera_dump"};
      char filename[100];
      snprintf(filename,
               sizeof(filename),
               "%s/input_%d.yuv",
               dir,
               result.reqNumber);
      FILE* fd;
      if ((fd= fopen(filename, "w"))) {
        size_t err = 2;
        err = fwrite(va, size, 1, fd);
        MY_LOGD("fwrite req(%d) result: %zu", result.reqNumber, err);
        fclose(fd);
      } else {
        MY_LOGE("cannot open the file:%s", filename);
      }
    }
    munmap(va, size);
    pHal3Plus->totalAvailableBufMap.emplace(
                                    (result.reqNumber+pHal3Plus->maxBuffers),
                                    iter->second);
    pHal3Plus->nativeAvailableBufMap.erase(iter);
    MY_LOGI("totalAvailableBufMap(size:%lu),"
            " nativeAvailableBufMap(size:%lu),reqNumber:%d",
            pHal3Plus->totalAvailableBufMap.size(),
            pHal3Plus->nativeAvailableBufMap.size(), result.reqNumber);
  }
  // dump result metadata for FD
  if (result.presult != nullptr) {
    auto mConverter = NSCam::IMetadataConverter::createInstance(
        NSCam::IDefaultMetadataTagSet::singleton()->getTagSet());
    NSCam::IMetadata metaResultOut;
    if (mConverter->convert(result.presult, metaResultOut)) {
      NSCam::IMetadata::IEntry const &faceReprocessResult = metaResultOut.entryFor(MTK_FACE_FEATURE_REPROCESS_RESULT);
      if (!faceReprocessResult.isEmpty()) {
        uint8_t *pResultByteArray = new unsigned char[sizeof(MtkCameraFaceMetadata)];
        if (faceReprocessResult.itemAt(0, pResultByteArray, sizeof(MtkCameraFaceMetadata), NSCam::Type2Type<uint8_t>())) {
          MtkCameraFaceMetadata *faceResult = reinterpret_cast<MtkCameraFaceMetadata *>(pResultByteArray);
          MY_LOGI("Get FD result meta success! number of face: %d", faceResult->number_of_faces);
          if (faceResult->number_of_faces > 0) {
            for (uint32_t i = 0; i < faceResult->number_of_faces; i++) {
              MY_LOGD("face[%d]: (%d, %d, %d, %d)", i,
                      faceResult->faces[i].rect[0],
                      faceResult->faces[i].rect[1],
                      faceResult->faces[i].rect[2],
                      faceResult->faces[i].rect[3]);
            }
          }
        } else {
          MY_LOGE("Get item from entry fail");
        }
      } else {
        MY_LOGE("faceReprocessResult is empty");
      }
    } else {
      MY_LOGE("Convert camera_metadata to IMetadata fail");
    }
  } else {
    MY_LOGI("no output metadata");
  }
  pHal3Plus->nativeCondition.notify_all();
  return 0;
}




