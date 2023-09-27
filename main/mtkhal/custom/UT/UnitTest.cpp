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
#define LOG_TAG "Mtk_Hal_Ut"

#include "main/mtkhal/custom/UT/UnitTest.h"

#include <dlfcn.h>
#include <faces.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "cutils/properties.h"
#include "json/json.h"
#include "linux/ion_4.12.h"
#include "linux/ion_drv.h"
#include "linux/mman-proprietary.h"
#include "mtkcam/def/ImageFormat.h"
#include "mtkcam/utils/metadata/IMetadataConverter.h"
#include "mtkcam/utils/metadata/IMetadataTagSet.h"
#include "mtkcam/utils/metadata/client/mtk_metadata_tag.h"
#include "mtkcam/utils/std/Log.h"
#include "mtkcam/utils/std/LogTool.h"
#include "mtkcam/utils/std/Trace.h"
#include "mtkcam/utils/std/ULog.h"
#include "ui/gralloc_extra.h"
#include "vndk/hardware_buffer.h"

#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)

#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)

#define DUMP_FOLDER "/data/vendor/camera_dump/"
#define JSON_FOLDER "/data/vendor/camera_dump/"
#define FUNC_START MY_LOGD("+");
#define FUNC_END MY_LOGD("-");
#define TME_STREAM_ID 255

CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);

extern "C" void runMtkHalTest(const std::shared_ptr<IPrinter> printer,
                              const std::vector<std::string>& options,
                              const std::vector<int32_t>& cameraIdList) {
  // ::android::sp<MtkHalTest> mtkHalTestInstance = new MtkHalTest();
  MtkHalTest* pMtkHalTestInstance = new MtkHalTest();
  if (!pMtkHalTestInstance) {
    MY_LOGE("cannot create MtkHalTest!");
  } else {
    MY_LOGD("mtkHalTestInstance->start(+)");
    pMtkHalTestInstance->start(printer, options, cameraIdList);
    // mtkHalTestInstance->requestExit();
    MY_LOGD("mtkHalTestInstance->start(-)");
  }
  pMtkHalTestInstance = nullptr;
  delete pMtkHalTestInstance;
  MY_LOGD("release MtkHalTest instance");
}

extern "C" void runMtkHalUt(const std::shared_ptr<IPrinter> printer,
                            const std::vector<std::string>& options,
                            const std::vector<int32_t>& cameraIdList) {
  ::android::sp<MtkHalUt> mtkHalUtInstance = new MtkHalUt();
  if (!mtkHalUtInstance) {
    MY_LOGE("cannot create MtkHalUt!");
  } else {
    MY_LOGD("mtkHalUtInstance->start(+)");
    mtkHalUtInstance->start(printer, options, cameraIdList);
    mtkHalUtInstance->requestExit();
    MY_LOGD("mtkHalUtInstance->start(-)");
  }
  mtkHalUtInstance = nullptr;
  MY_LOGD("release MtkHalUt instance");
}

int32_t getDebugLevel() {
  return ::property_get_int32("vendor.debug.hal3ut.loglevel", 1);
}

template <typename T>
static std::string intToHexString(T i) {
  std::stringstream ss;
  ss << "0x" << std::hex << i;
  return ss.str();
}

static std::string toString(std::shared_ptr<mcam::IImageBufferHeap> s) {
  std::ostringstream oss;
  if (s == nullptr) {
    oss << "mcam::IImageBufferHeap:{ nullptr }";
    return oss.str();
  }
  // s->lockBuf("toString",mcam::eBUFFER_USAGE_SW_READ_OFTEN);
  oss << "mcam::IImageBufferHeap:{"
      << " .get()=" << intToHexString(s.get())
      << " width=" << std::to_string(s->getImgSize().w)
      << " height=" << std::to_string(s->getImgSize().h)
      << " format=" << intToHexString(s->getImgFormat())
      << " stride[0]=" << std::to_string(s->getBufStridesInBytes(0))
      << " stride[1]="
      << (s->getPlaneCount() > 1 ? std::to_string(s->getBufStridesInBytes(1))
                                 : std::to_string(0))
      << " stride[2]="
      << (s->getPlaneCount() > 2 ? std::to_string(s->getBufStridesInBytes(2))
                                 : std::to_string(0))
      //    << " VA=" << intToHexString(s->getBufVA(0))
      //    << " PA=" << intToHexString(s->getBufVA(0))
      << "}";
  // s->unlockBuf("toString");
  return oss.str();
}

static std::string toString(StreamBuffer& s) {
  std::ostringstream oss;
  oss << "StreamBuffer:{"
      << " id=" << std::to_string(s.streamId)
      << " bufferId=" << std::to_string(s.bufferId)
      << " status=" << std::to_string(static_cast<uint32_t>(s.status))
      << " bufferSettings.count=" << std::to_string(s.bufferSettings.count())
      << " buffer=" << toString(s.buffer) << "}";
  return oss.str();
}

static std::string toString(std::vector<StreamBuffer>& ss) {
  std::ostringstream oss;
  for (auto s : ss) {
    oss << toString(s);
  }
  return oss.str();
}

static std::string toString(CaptureRequest& r) {
  std::ostringstream oss;
  oss << "CaptureRequest:{"
      << " frameNo=" << r.frameNumber
      << " inputBuffers=" << toString(r.inputBuffers)
      << " outputBuffers=" << toString(r.outputBuffers)
      << " settings.size=" << std::to_string(r.settings.count())
      << " halSettings.size=" << std::to_string(r.settings.count()) << "}";
  return oss.str();
}

static std::string toString(Stream& stream) {
  auto queryStreamSource = [](IMetadata& meta) {
    MINT32 source = 0;
    bool b = IMetadata::getEntry(&meta, MTK_HALCORE_STREAM_SOURCE, source, 0);
    if (!b)
      return 0;
    else
      return source;
  };
  std::ostringstream oss;
  oss << "Stream:{"
      << " id=" << std::to_string(stream.id) << " streamType="
      << std::to_string(static_cast<uint32_t>(stream.streamType))
      << " width=" << std::to_string(stream.width)
      << " height=" << std::to_string(stream.height)
      << " format=" << intToHexString(stream.format)
      << " usage=" << std::to_string(stream.usage) << " dataSpace="
      << std::to_string(static_cast<uint32_t>(stream.dataSpace))
      << " transform=" << std::to_string(stream.transform)
      << " physicalCameraId=" << std::to_string(stream.physicalCameraId)
      << " bufferSize=" << std::to_string(stream.bufferSize)
      << " settings.size=" << std::to_string(stream.settings.count())
      << " settings.MTK_HALCORE_STREAM_SOURCE="
      << queryStreamSource(stream.settings) << " bufPlanes[0].stride="
      << std::to_string(stream.bufPlanes.planes[0].rowStrideInBytes)
      << " bufPlanes[1].stride="
      << std::to_string(stream.bufPlanes.planes[1].rowStrideInBytes)
      << " bufPlanes[2].stride="
      << std::to_string(stream.bufPlanes.planes[2].rowStrideInBytes) << "}";
  return oss.str();
}

static void toLog(std::string msg, std::vector<Stream>& streams) {
  CAM_ULOGMD("[%s] %s", __FUNCTION__, msg.c_str());
  for (auto stream : streams) {
    CAM_ULOGMD("[%s] %s", __FUNCTION__, toString(stream).c_str());
  }
}

static void toLog(std::string msg, std::vector<CaptureRequest>& requests) {
  CAM_ULOGMD("[%s] %s", __FUNCTION__, msg.c_str());
  for (auto r : requests) {
    CAM_ULOGMD("[%s] %s", __FUNCTION__, toString(r).c_str());
  }
}

template <typename T>
bool trySetVectorToMetadata(NSCam::IMetadata* pMetadata,
                            MUINT32 tag,
                            std::vector<T> data) {
  NSCam::IMetadata::IEntry entry(tag);
  for (auto& val : data) {
    entry.push_back(val, NSCam::Type2Type<T>());
  }
  int err = pMetadata->update(tag, entry);
  return (err == 0) ? true : false;
}

#define CHECK_IS_MEMBER(node, key)               \
  if (!node.isMember(key)) {                     \
    MY_LOGE("NODE:%s not found in .json!", key); \
    return MUST_FILLED_INFO_NOT_FOUND;           \
  }

#define CONVERT_TYPE_TO_STRING(_type) g_JsonTypeMap.find(_type)->second

#define CHECK_TYPE(node, key, _type)                                  \
  if (node[key].type() != _type) {                                    \
    MY_LOGE("NODE:%s is type (%s), not (%s) type in .json", key,      \
            CONVERT_TYPE_TO_STRING(node[key].type()).c_str(), #_type);        \
    return NOT_EXPECTED_TYPE;                                         \
  }

#define CHECK_2TYPES(node, key, _type1, _type2)                               \
  if (node[key].type() != _type1 && node[key].type() != _type2) {             \
    MY_LOGE("NODE:%s is type (%s), not (%s) or (%s) type in .json",           \
            key, CONVERT_TYPE_TO_STRING(node[key].type()).c_str(),            \
            #_type1, #_type2);                                                \
    return NOT_EXPECTED_TYPE;                                                 \
  }

#define CHECK_3TYPES(node, key, _type1, _type2, _type3)                       \
  if (node[key].type() != _type1 && node[key].type() != _type2 &&             \
      node[key].type() != _type3) {                                           \
    MY_LOGE(                                                                  \
        "NODE:%s is type (%s), not (%s) or (%s) or (%s) type in .json",       \
        key, CONVERT_TYPE_TO_STRING(node[key].type()).c_str(),                \
        #_type1, #_type2, #_type3);                                           \
    return NOT_EXPECTED_TYPE;                                                 \
  }

#define CHECK_4TYPES(node, key, _type1, _type2, _type3, _type4)             \
  if (node[key].type() != _type1 && node[key].type() != _type2 &&           \
      node[key].type() != _type3 && node[key].type() != _type4) {           \
    MY_LOGE(                                                                \
        "NODE:%s is type (%s), not (%s) or (%s) or (%s) or (%s) in .json",  \
        key, CONVERT_TYPE_TO_STRING(node[key].type()).c_str(),              \
        #_type1, #_type2, #_type3, #_type4);                                \
    return NOT_EXPECTED_TYPE;                                               \
  }

#define CHECK_AND_GET_STRING(node, key, result) \
  CHECK_IS_MEMBER(node, key);                   \
  CHECK_TYPE(node, key, Json::stringValue);     \
  result = node[key].asString();

#define CHECK_AND_GET_STRING_ALLOW_EMPTY(node, key, result) \
  if (node.isMember(key)) {                                 \
    CHECK_TYPE(node, key, Json::stringValue);               \
    result = node[key].asString();                          \
  }

#define CHECK_AND_GET_INT(node, key, result) \
  CHECK_IS_MEMBER(node, key);                \
  CHECK_TYPE(node, key, Json::intValue);     \
  result = node[key].asInt();

#define TRY_TO_GET_INT(node, key, result) \
  CHECK_2TYPES(node, key, Json::intValue, Json::nullValue);     \
  result = node[key].asInt();

#define CHECK_AND_GET_UINT(node, key, result) \
  CHECK_IS_MEMBER(node, key);                 \
  CHECK_TYPE(node, key, Json::uintValue);     \
  result = node[key].asUInt();

#define CHECK_AND_GET_HEX_OR_UINT(node, key, result)                          \
  CHECK_IS_MEMBER(node, key);                                                 \
  CHECK_4TYPES(node, key, Json::stringValue, Json::uintValue, Json::intValue, \
               Json::intValue);                                               \
  if (node[key].isString()) {                                                 \
    string s_tmp = node[key].asString();                                      \
    result = strtoul(s_tmp.c_str(), NULL, 16);                                \
  } else {                                                                    \
    result = node[key].asUInt();                                              \
  }

#define CHECK_AND_GET_DIGITAL_NUMBER_ARRAY(node, key, result)                 \
  CHECK_IS_MEMBER(node, key);                                                 \
  CHECK_4TYPES(node, key, Json::stringValue, Json::uintValue, Json::intValue, \
               Json::arrayValue);                                             \
  if (node[key].isString()) {                                                 \
    string s_tmp = node[key].asString();                                      \
    result.push_back(strtoul(s_tmp.c_str(), NULL, 16));                       \
  } else if (node[key].isUInt()) {                                            \
    result.push_back(node[key].asUInt());                                     \
  } else if (node[key].isInt()) {                                             \
    result.push_back(node[key].asInt());                                      \
  } else {                                                                    \
    for (auto& s : node[key]) {                                               \
      if (s.isString()) {                                                     \
        string s_tmp = s.asString();                                          \
        result.push_back(strtoul(s_tmp.c_str(), NULL, 16));                   \
      } else if (s.isUInt()) {                                                \
        result.push_back(s.asUInt());                                         \
      } else if (s.isInt()) {                                                 \
        result.push_back(s.asInt());                                          \
      } else {                                                                \
        MY_LOGE("NODE:%s is not INT/UINT or hex string! : (%s)", key,         \
                CONVERT_TYPE_TO_STRING(s.type()).c_str());                    \
        return NOT_EXPECTED_TYPE;                                             \
      }                                                                       \
    }                                                                         \
  }

#define CHECK_AND_GET_GENERAL_ARRAY_ALLOW_EMPTY(node, key, result)  \
  if (node.isMember(key)) {                                         \
    CHECK_3TYPES(node, key, Json::stringValue, Json::uintValue,     \
                 Json::arrayValue);                                 \
    if (node[key].isString()) {                                     \
      result.push_back(node[key].asString());                       \
    } else if (node[key].isUInt()) {                                \
      result.push_back(node[key].asString());                       \
    } else if (node[key].isInt()) {                                 \
      result.push_back(node[key].asString());                       \
    } else {                                                        \
      for (auto& s : node[key]) {                                   \
        if (s.isString()) {                                         \
          result.push_back(s.asString());                           \
        } else if (s.isUInt()) {                                    \
          result.push_back(s.asString());                           \
        } else if (s.isInt()) {                                     \
          result.push_back(std::to_string(s.asInt()));              \
        } else {                                                    \
          MY_LOGE("NODE:%s is not INT/UINT or hex string! : ", key, \
                  CONVERT_TYPE_TO_STRING(s.type()).c_str());        \
          return NOT_EXPECTED_TYPE;                                 \
        }                                                           \
      }                                                             \
    }                                                               \
  }

#define CHECK_AND_GET_METADATA(m, meta)                               \
  CHECK_AND_GET_HEX_OR_UINT(m, "tagId", meta.tagId);                  \
  CHECK_AND_GET_STRING(m, "tagName", meta.tagName);                   \
  CHECK_AND_GET_STRING(m, "tagType", meta.tagType);                   \
  CHECK_AND_GET_DIGITAL_NUMBER_ARRAY(m, "tagValues", meta.tagValues); \
  CHECK_AND_GET_GENERAL_ARRAY_ALLOW_EMPTY(m, "tagValuesName",         \
                                          meta.tagValuesName);

#define PRINT_DATA_TO_OSS(data) \
  oss << #data << "=" << static_cast<int>(r.data) << ", ";

#define PRINT_DATA_STRING_TO_OSS(data) oss << #data << "=" << r.data << ", ";

#define PRINT_DATA_TO_OSS_WITH_HEX(data)                                     \
  oss << #data << "=0x" << std::setiosflags(std::ios::uppercase) << std::hex \
      << r.data << ", " << std::resetiosflags(std::ios::uppercase)           \
      << std::dec;

#define PRINT_VECTOR_TO_OSS(data)     \
  {                                   \
    int i = 0;                        \
    for (auto& s : r.data) {          \
      oss << #data << "[" << i << "]" \
          << "=" << s << ", ";        \
      i++;                            \
    }                                 \
  }

string toString_BufPlanes(NSCam::BufPlanes& bufPlanes) {
  std::ostringstream oss;
  oss << "BufPlanes:{";
  for (auto i = 0; i < bufPlanes.count; i++) {
    oss << " planes[" << i
        << "]:(sizeInBytes=" << bufPlanes.planes[i].sizeInBytes
        << ", rowStrideInBytes=" << bufPlanes.planes[i].rowStrideInBytes << ")";
  }
  oss << "}_BufPlanes";
  return oss.str();
}

string toString_Stream(Stream& r) {
  std::ostringstream oss;
  oss << "Stream:{";
  PRINT_DATA_TO_OSS_WITH_HEX(id);
  // PRINT_DATA_TO_OSS(streamType);
  oss << "streamType"
      << "=" << static_cast<uint32_t>(r.streamType) << ", ";
  PRINT_DATA_TO_OSS(width);
  PRINT_DATA_TO_OSS(height);
  PRINT_DATA_TO_OSS_WITH_HEX(format);
  oss << toString_BufPlanes(r.bufPlanes);
  PRINT_DATA_TO_OSS(usage);
  PRINT_DATA_TO_OSS(dataSpace);
  PRINT_DATA_TO_OSS(transform);
  PRINT_DATA_TO_OSS(physicalCameraId);
  PRINT_DATA_TO_OSS(bufferSize);
  oss << "settings.count()=" << r.settings.count();
  oss << "}_Stream";
  return oss.str();
}

string toString_StreamBuffer(StreamBuffer& r) {
  std::ostringstream oss;
  oss << "StreamBuffer:{";
  PRINT_DATA_TO_OSS_WITH_HEX(streamId);
  PRINT_DATA_TO_OSS(bufferId);
  oss << "buffer=" << r.buffer.get();
  PRINT_DATA_TO_OSS(status);
  PRINT_DATA_TO_OSS(acquireFenceFd);
  PRINT_DATA_TO_OSS(releaseFenceFd);
  oss << "bufferSettings.count()=" << r.bufferSettings.count();
  oss << "}_StreamBuffer";
  return oss.str();
}

string toString_onlyMetaSetting(MetadataSetting const& r) {
  std::ostringstream oss;
  oss << "MetadataSetting:{";
  PRINT_DATA_TO_OSS_WITH_HEX(tagId);
  PRINT_DATA_STRING_TO_OSS(tagName);
  PRINT_DATA_STRING_TO_OSS(tagType);
  PRINT_VECTOR_TO_OSS(tagValues);
  PRINT_VECTOR_TO_OSS(tagValuesName);
  oss << "}_MetadataSetting";
  return oss.str();
}

string toString_onlyStreamInfo(StreamSetting const& r) {
  std::ostringstream oss;
  oss << "StreamSetting:{";
  PRINT_DATA_TO_OSS_WITH_HEX(streamId);
  PRINT_DATA_TO_OSS_WITH_HEX(format);
  PRINT_DATA_TO_OSS(width);
  PRINT_DATA_TO_OSS(height);
  PRINT_DATA_TO_OSS(stride);
  PRINT_DATA_TO_OSS_WITH_HEX(usage);
  PRINT_DATA_TO_OSS(dataSpace);
  PRINT_DATA_TO_OSS(transform);
  PRINT_DATA_TO_OSS(physicalCameraId);
  oss << "}_StreamSetting";
  return oss.str();
}

string toString_onlyRequestInfo(RequestSetting const& r) {
  std::ostringstream oss;
  oss << "StreamInfo:{";
  oss << "frameNo(size:" << r.frameNo.size() << ") = [";
  for (const auto& s : r.frameNo) {
    oss << s << ", ";
  }
  oss << "], ";
  oss << "outputStreams(size:" << r.outputStreams.size() << ") = [";
  for (const auto& s : r.outputStreams) {
    oss << "id:" << s.streamId << ", ";
    for (const auto& setting : s.streamSettings) {
      oss << "setting:" << toString_onlyMetaSetting(setting) << ", ";
    }
  }
  oss << "]";
  oss << "}_StreamSetting";
  return oss.str();
}

void calcuBufPlane(int32_t width,
                   int32_t height,
                   int32_t stride,
                   EImageFormat format,
                   NSCam::BufPlanes& bufPlanes) {
#define addBufPlane(idx, planes, height, stride) \
  do {                                           \
    planes[idx] = {                              \
        .sizeInBytes = (height * stride),        \
        .rowStrideInBytes = stride,              \
    };                                           \
  } while (0)
  switch (format) {
    case NSCam::EImageFormat::eImgFmt_BLOB:
    case NSCam::EImageFormat::eImgFmt_JPEG:
      bufPlanes.count = 1;
      addBufPlane(0, bufPlanes.planes, 1, (size_t)stride);
      break;
    case NSCam::EImageFormat::eImgFmt_YUY2:  // direct yuv
    case NSCam::EImageFormat::eImgFmt_BAYER10:
    case NSCam::EImageFormat::eImgFmt_BAYER12:
    case NSCam::EImageFormat::eImgFmt_BAYER10_UNPAK:
    case NSCam::EImageFormat::eImgFmt_BAYER12_UNPAK:
    case NSCam::EImageFormat::eImgFmt_FG_BAYER10:
    case NSCam::EImageFormat::eImgFmt_FG_BAYER12:
    case NSCam::EImageFormat::eImgFmt_BAYER8:  // LCSO
    case NSCam::EImageFormat::eImgFmt_STA_BYTE:
    case NSCam::EImageFormat::eImgFmt_STA_2BYTE:          // LCSO with LCE3.0
    case NSCam::EImageFormat::eImgFmt_BAYER16_APPLY_LSC:  // for AF input
                                                          // raw in ISP6.0
    case NSCam::EImageFormat::eImgFmt_Y8:
      bufPlanes.count = 1;
      addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
      break;
    case NSCam::EImageFormat::eImgFmt_NV16:  // direct yuv 2 plane
      bufPlanes.count = 2;
      addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
      addBufPlane(1, bufPlanes.planes, height, (size_t)stride);
      break;
    case NSCam::EImageFormat::eImgFmt_MTK_YUV_P010:  // direct yuv 10bit, 2
    case NSCam::EImageFormat::eImgFmt_MTK_YVU_P010:  // plane
    case NSCam::EImageFormat::eImgFmt_MTK_YUV_P210:
    case NSCam::EImageFormat::eImgFmt_MTK_YVU_P210:
    case NSCam::EImageFormat::eImgFmt_MTK_YUV_P012:
    case NSCam::EImageFormat::eImgFmt_MTK_YVU_P012:
    case NSCam::EImageFormat::eImgFmt_YUV_P010:  // direct yuv 10bit, 2
    case NSCam::EImageFormat::eImgFmt_YUV_P210:  // plane
    case NSCam::EImageFormat::eImgFmt_YUV_P012:
    case NSCam::EImageFormat::eImgFmt_YVU_P010:
    case NSCam::EImageFormat::eImgFmt_YVU_P210:
    case NSCam::EImageFormat::eImgFmt_YVU_P012:
    case NSCam::EImageFormat::eImgFmt_NV21:
    case NSCam::EImageFormat::eImgFmt_NV12:
      bufPlanes.count = 2;
      addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
      addBufPlane(1, bufPlanes.planes, (height >> 1), (size_t)stride);
      break;
    case NSCam::EImageFormat::eImgFmt_YV12:
      bufPlanes.count = 3;
      addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
      addBufPlane(1, bufPlanes.planes, (height >> 1), (size_t)stride >> 1);
      addBufPlane(2, bufPlanes.planes, (height >> 1), (size_t)stride >> 1);
      break;

    case NSCam::EImageFormat::eImgFmt_WARP_2PLANE:
          bufPlanes.count = 2;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          addBufPlane(1, bufPlanes.planes, height, (size_t)stride);
      break;
    default:
      MY_LOGA("unsupported format:%d - imgSize:%dx%d stride:%zu", format, width,
              height, (size_t)stride);
      break;
  }
#undef addBufPlane
}

void JSonUtil::dumpJsonData(JsonData const& jsonData) {
  MY_LOGD("start to dump JsonData");
  MY_LOGD("<filePath> : %s", jsonData.filePath.c_str());
  MY_LOGD("<basic info>");
  MY_LOGD("  keyName = %s", jsonData.keyName.c_str());
  MY_LOGD("  caseName = %s", jsonData.caseName.c_str());
  MY_LOGD("  caseType = %s", jsonData.caseType.c_str());
  MY_LOGD("  cameraDeviceId = %d", jsonData.cameraDeviceId);
  MY_LOGD("  rhsDeviceId = %d", jsonData.rhsDeviceId);
  MY_LOGD("  operationMode = %d", jsonData.operationMode);
  MY_LOGD("  streamId_ME = %d", jsonData.streamId_ME);
  MY_LOGD("  streamId_TME = %d", jsonData.streamId_TME);
  MY_LOGD("  streamId_warpMap = %d", jsonData.streamId_warpMap);
  MY_LOGD("<LHS part - config - outputStreams> (size:%zu)",
          jsonData.Lhs.configSetting.outputStreams.size());
  int i = 0;
  int j = 0;
  for (auto const& s : jsonData.Lhs.configSetting.outputStreams) {
    MY_LOGD("  [%d]%s", i, toString_onlyStreamInfo(s).c_str());
    i++;
    j = 0;
    MY_LOGD("    Settings (size:%zu)", s.settings.size());
    for (auto const& m : s.settings) {
      MY_LOGD("    (%d)%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
  }
  MY_LOGD("<LHS part - config - sessionParams> (size:%zu)",
          jsonData.Lhs.configSetting.sessionParams.size());
  i = 0;
  for (auto const& m : jsonData.Lhs.configSetting.sessionParams) {
    MY_LOGD("  [%d]%s", i, toString_onlyMetaSetting(m).c_str());
    i++;
  }
  MY_LOGD("<LHS part - request> (size:%zu)",
          jsonData.Lhs.requestSettings.size());
  i = 0;
  for (auto const& r : jsonData.Lhs.requestSettings) {
    MY_LOGD("  [%d]Request:", i);
    std::ostringstream oss;
    oss << "frameNo(size:" << r.frameNo.size() << ") = [";
    for (const auto& s : r.frameNo) {
      oss << s << ", ";
    }
    oss << "]";
    MY_LOGD("    %s", oss.str().c_str());
    MY_LOGD("      outputStreams(size:%d)", r.outputStreams.size());
    for (const auto& s : r.outputStreams) {
      MY_LOGD("        StreamId:%d", s.streamId);
      j = 0;
      for (const auto& setting : s.streamSettings) {
        MY_LOGD("          [%d]%s", j,
                toString_onlyMetaSetting(setting).c_str());
        j++;
      }
    }
    //
    i++;
    MY_LOGD("    appMetadatas (size:%zu)", r.appMetadatas.size());
    j = 0;
    for (const auto& m : r.appMetadatas) {
      MY_LOGD("      (%d)%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
    j = 0;
    MY_LOGD("    hallMetadatas (size:%zu)", r.halMetadatas.size());
    for (const auto& m : r.halMetadatas) {
      MY_LOGD("      (%d)%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
  }
  //
  MY_LOGD("<RHS part - config - inputStreams> (size:%zu)",
          jsonData.Rhs.configSetting.inputStreams.size());
  i = 0;
  for (auto const& s : jsonData.Rhs.configSetting.inputStreams) {
    MY_LOGD("  [%d]%s", i, toString_onlyStreamInfo(s).c_str());
    i++;
    MY_LOGD("    Settings (size:%zu)", s.settings.size());
    j = 0;
    for (auto const& m : s.settings) {
      MY_LOGD("    [%d]%s", j, toString_onlyMetaSetting(m).c_str());
    }
  }
  MY_LOGD("<RHS part - config - outputStreams> (size:%zu)",
          jsonData.Rhs.configSetting.outputStreams.size());
  i = 0;
  for (auto const& s : jsonData.Rhs.configSetting.outputStreams) {
    MY_LOGD("  [%d]%s", i, toString_onlyStreamInfo(s).c_str());
    i++;
    MY_LOGD("    Settings (size:%zu)", s.settings.size());
    j = 0;
    for (auto const& m : s.settings) {
      MY_LOGD("    (%d)%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
  }
  MY_LOGD("<RHS part - config - sessionParams> (size:%zu)",
          jsonData.Rhs.configSetting.sessionParams.size());
  i = 0;
  for (auto const& m : jsonData.Rhs.configSetting.sessionParams) {
    MY_LOGD("  [%d]%s", i, toString_onlyMetaSetting(m).c_str());
    i++;
  }
  MY_LOGD("<RHS part - request> (size:%zu)",
          jsonData.Rhs.requestSettings.size());
  i = 0;
  for (auto const& r : jsonData.Rhs.requestSettings) {
    MY_LOGD("  [%d]Request:", i);
    std::ostringstream oss;
    oss << "frameNo(size:" << r.frameNo.size() << ") = [";
    for (const auto& s : r.frameNo) {
      oss << s << ", ";
    }
    oss << "]";
    MY_LOGD("    %s", oss.str().c_str());
    MY_LOGD("      inputStreams(size:%d)", r.inputStreams.size());
    for (const auto& s : r.inputStreams) {
      MY_LOGD("        StreamId:%d", s.streamId);
      j = 0;
      for (const auto& setting : s.streamSettings) {
        MY_LOGD("          [%d]%s", j,
                toString_onlyMetaSetting(setting).c_str());
        j++;
      }
    }
    MY_LOGD("      outputStreams(size:%d)", r.outputStreams.size());
    for (const auto& s : r.outputStreams) {
      MY_LOGD("        StreamId:%d", s.streamId);
      j = 0;
      for (const auto& setting : s.streamSettings) {
        MY_LOGD("          [%d]%s", j,
                toString_onlyMetaSetting(setting).c_str());
        j++;
      }
    }
    //
    MY_LOGD("    appMetadatas (size:%zu)", r.appMetadatas.size());
    j = 0;
    for (const auto& m : r.appMetadatas) {
      MY_LOGD("      [%d]%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
    MY_LOGD("    hallMetadatas (size:%zu)", r.halMetadatas.size());
    j = 0;
    for (const auto& m : r.halMetadatas) {
      MY_LOGD("      [%d]%s", j, toString_onlyMetaSetting(m).c_str());
      j++;
    }
    i++;
  }
}

void MtkHalData::dumpData() {
  // LHS config - basic
  MY_LOGD("<LHS configSetting>");
  MY_LOGD("  operationMode=%d", lhsData.configSetting.operationMode);
  MY_LOGD("  streamConfigCounter=%d",
          lhsData.configSetting.streamConfigCounter);
  // LHS config - streams
  int i = 0;
  int j = 0;
  MY_LOGD("<LHS configSetting - streams> (size:%d)",
          lhsData.configSetting.streams.size());
  for (auto& s : lhsData.configSetting.streams) {
    MY_LOGD("  [%d]%s", i, toString_Stream(s).c_str());
    i++;
  }
  // LHS config - session params
  MY_LOGD("  sessionParams.count()=%d",
          lhsData.configSetting.sessionParams.count());
  // LHS request - specific request data
  MY_LOGD("<LHS specific request>");
  i = 0;
  for (auto& it : lhsData.specificRequestsMap) {
    CaptureRequest r = it.second;
    MY_LOGD(
        "  [%d]frameNo=%d, app_metadata.count()=%d, hal_metadata.count()=%d", i,
        r.frameNumber, r.settings.count(), r.halSettings.count());
    // output streamBuf
    MY_LOGD("    <LHS repeat request - outputBuffers>");
    j = 0;
    for (auto& s : r.outputBuffers) {
      MY_LOGD("    [%d]%s", j, toString_StreamBuffer(s).c_str());
      j++;
    }
    i++;
  }
  // LHS request - repeat request data
  MY_LOGD("<LHS repeat request>");
  MY_LOGD("  [%d]frameNo=%d, app_metadata.count()=%d, hal_metadata.count()=%d",
          i, lhsData.repeatRequest.frameNumber,
          lhsData.repeatRequest.settings.count(),
          lhsData.repeatRequest.halSettings.count());
  // output streamBuf
  MY_LOGD("    <LHS repeat request - outputBuffers>");
  i = 0;
  for (auto& s : lhsData.repeatRequest.outputBuffers) {
    MY_LOGD("    [%d]%s", i, toString_StreamBuffer(s).c_str());
    i++;
  }

  //////////////////////////////////////////

  // RHS config - basic
  MY_LOGD("<RHS configSetting>");
  MY_LOGD("  operationMode=%d", rhsData.configSetting.operationMode);
  MY_LOGD("  streamConfigCounter=%d",
          rhsData.configSetting.streamConfigCounter);
  // RHS config - streams
  i = 0;
  j = 0;
  for (auto& s : rhsData.configSetting.streams) {
    MY_LOGD("  [%d]%s", i, toString_Stream(s).c_str());
    i++;
  }
  // RHS config - session params
  MY_LOGD("  sessionParams.count()=%d",
          rhsData.configSetting.sessionParams.count());
  // RHS request - specific request data
  MY_LOGD("<RHS specific request>");
  i = 0;
  for (auto& it : rhsData.specificRequestsMap) {
    CaptureRequest r = it.second;
    MY_LOGD(
        "  [%d]frameNo=%d, app_metadata.count()=%d, hal_metadata.count()=%d", i,
        r.frameNumber, r.settings.count(), r.halSettings.count());
    // input streamBuf
    MY_LOGD("    <RHS specific request - inputBuffers>");
    j = 0;
    for (auto& s : r.inputBuffers) {
      MY_LOGD("    [%d]%s", j, toString_StreamBuffer(s).c_str());
      j++;
    }
    // output streamBuf
    MY_LOGD("    <RHS specific request - outputBuffers>");
    j = 0;
    for (auto& s : r.outputBuffers) {
      MY_LOGD("    [%d]%s", j, toString_StreamBuffer(s).c_str());
      j++;
    }
    i++;
  }
  // RHS request - repeat request data
  MY_LOGD("<RHS repeat request>");
  MY_LOGD("  [%d]frameNo=%d, app_metadata.count()=%d, hal_metadata.count()=%d",
          i, rhsData.repeatRequest.frameNumber,
          rhsData.repeatRequest.settings.count(),
          rhsData.repeatRequest.halSettings.count());
  // input streamBuf
  MY_LOGD("    <RHS repeat request - inputBuffers>");
  j = 0;
  for (auto& s : rhsData.repeatRequest.inputBuffers) {
    MY_LOGD("    [%d]%s", j, toString_StreamBuffer(s).c_str());
    j++;
  }
  // output streamBuf
  MY_LOGD("    <RHS repeat request - outputBuffers>");
  i = 0;
  for (auto& s : rhsData.repeatRequest.outputBuffers) {
    MY_LOGD("    [%d]%s", i, toString_StreamBuffer(s).c_str());
    i++;
  }
}

bool MtkHalData::isValid() {
  if (mbTransformed) {
    return true;
  }
  return false;
}

bool MtkHalData::getLhsConfigData(StreamConfiguration& setting) {
  if (!isValid()) {
    MY_LOGE("Need to call TransformFromJsonData() first!");
    return false;
  }
  setting = lhsData.configSetting;
  return true;
}

bool MtkHalData::getRhsConfigData(StreamConfiguration& setting) {
  if (!isValid()) {
    MY_LOGE("Need to call TransformFromJsonData() first!");
    return false;
  }
  setting = rhsData.configSetting;
  return true;
}

bool MtkHalData::getLhsRequestData(int frameNo, CaptureRequest& capReq) {
  if (!isValid()) {
    MY_LOGE("Need to call TransformFromJsonData() first!");
    return false;
  }
  if (lhsData.specificRequestsMap.find(frameNo) !=
      lhsData.specificRequestsMap.end()) {
    // get captureRequest
    capReq = lhsData.specificRequestsMap.at(frameNo);
    // get output image buffer for captureRequest
    for (StreamBuffer& buf : capReq.outputBuffers) {
      if (buf.buffer == nullptr) {
        AllocateBufferInfo bufInfo;
        if (getAllocateBufferInfo_LhsOutput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(specific) LHS outStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(specific) LHS outStream: frameNo:%d, streamId:0x%X not in "
              "mLhsOutImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        MY_LOGD("(specific) LHS:frameNo:%d streamId:0x%X buffer is ready",
                frameNo, buf.streamId);
      }
    }
  } else {
    // get captureRequest
    capReq = lhsData.repeatRequest;
    // assing frameNo for repeat request
    capReq.frameNumber = frameNo;
    // get output image buffer for captureRequest
    for (StreamBuffer& buf : capReq.outputBuffers) {
      if (buf.buffer == nullptr) {
        AllocateBufferInfo bufInfo;
        if (getAllocateBufferInfo_LhsOutput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(repeat) LHS outStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(repeat) LHS outStream: frameNo:%d, streamId:0x%X not in "
              "mLhsOutImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        MY_LOGD(
            "(repeat) LHS outStream: frameNo:%d streamId:0x%X buffer is ready",
            frameNo, buf.streamId);
      }
    }
  }
  return true;
}

bool MtkHalData::getRhsRequestData(int frameNo, CaptureRequest& capReq) {
  if (!isValid()) {
    MY_LOGE("Need to call TransformFromJsonData() first!");
    return false;
  }
  MY_LOGD("frameNo:%d", frameNo);
  if (rhsData.specificRequestsMap.find(frameNo) !=
      rhsData.specificRequestsMap.end()) {
    MY_LOGD("in specificRequests");
    // get captureRequest
    capReq = rhsData.specificRequestsMap.at(frameNo);
    // get input image buffer for captureRequest (from LHS output image buffer)
    MY_LOGD("capReq.inputBuffers.size(%zu)", capReq.inputBuffers.size());
    // for (StreamBuffer& buf : capReq.inputBuffers) {
    for (auto it = capReq.inputBuffers.begin();
         it != capReq.inputBuffers.end();) {
      //
      auto& buf = *it;
      AllocateBufferInfo bufInfo;
      if (frameNo == 0 &&
          buf.streamId ==
              mStreamId_TME) {  // first frame, TME(0xFF) must be null
        MY_LOGD("frame==0, streamId==0xFF, first frame, TME(0x%X) must be null",
                mStreamId_TME);
        it = capReq.inputBuffers.erase(it);
        continue;
      } else if (buf.streamId ==
                 mStreamId_TME) {  // TME must use previous frame's ME buffer
        if (getAllocateBufferInfo_RhsInput(frameNo - 1, mStreamId_ME,
                                           bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(specific) RHS inStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(specific) RHS inStream: frameNo:%d, streamId:0x%X not in "
              "mRhsInImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        if (getAllocateBufferInfo_RhsInput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(specific) RHS inStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(specific) RHS inStream: frameNo:%d, streamId:0x%X not in "
              "mRhsInImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      }

      if (buf.streamId == mStreamId_warpMap) {
        MY_LOGD("find mStreamId_warpMap %d", mStreamId_warpMap);
        auto imgSize = capReq.inputBuffers[0].buffer->getImgSize();
        int64_t wpeOutW = imgSize.w, wpeOutH = imgSize.h;
        if (IMetadata::getEntry(&capReq.settings,
                     MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, wpeOutW, 0) != true)
          MY_LOGD("getEntry MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE Fail");
        if (IMetadata::getEntry(&capReq.settings,
                     MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, wpeOutH, 1) != true)
          MY_LOGD("getEntry MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE Fail");

        if (mUseWPEtoCrop) {
          writeWarpMap(buf, imgSize.w, imgSize.h, wpeOutW, wpeOutH);
        } else {
          writeWarpMap(buf, imgSize.w, imgSize.h);
        }
      }
      it++;
    }
    // get output image buffer for captureRequest
    MY_LOGD("capReq.outputBuffers.size(%zu)", capReq.outputBuffers.size());
    for (StreamBuffer& buf : capReq.outputBuffers) {
      if (buf.buffer == nullptr) {
        AllocateBufferInfo bufInfo;
        if (getAllocateBufferInfo_RhsOutput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(specific) RHS outStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(specific) RHS outStream: frameNo:%d, streamId:0x%X not in "
              "mRhsOutImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        MY_LOGD(
            "(specific) RHS outStream:frameNo:%d streamId:0x%X buffer is ready",
            frameNo, buf.streamId);
      }
    }
  } else {
    MY_LOGD("in repeatRequests");
    // get captureRequest
    capReq = rhsData.repeatRequest;
    // assing frameNo for repeat request
    capReq.frameNumber = frameNo;
    // get input image buffer for captureRequest (from LHS output image buffer)
    MY_LOGD("capReq.inputBuffers.size(%zu)", capReq.inputBuffers.size());
    for (auto it = capReq.inputBuffers.begin();
         it != capReq.inputBuffers.end();) {
      //
      auto& buf = *it;
      AllocateBufferInfo bufInfo;
      if (frameNo == 0 &&
          buf.streamId ==
              mStreamId_TME) {  // first frame, TME(0xFF) must be null
        MY_LOGD("frame==0, streamId==0xFF, first frame, TME(0x%X) must be null",
                mStreamId_TME);
        it = capReq.inputBuffers.erase(it);
        continue;
      } else if (buf.streamId ==
                 mStreamId_TME) {  // TME must use previous frame's ME buffer
        if (getAllocateBufferInfo_RhsInput(frameNo - 1, mStreamId_ME,
                                           bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(repeat) RHS inStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(repeat) RHS inStream: frameNo:%d, streamId:0x%X not in "
              "mRhsInImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        if (getAllocateBufferInfo_RhsInput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(repeat) RHS inStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(repeat) RHS inStream: frameNo:%d, streamId:0x%X not in "
              "mRhsInImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      }

      if (buf.streamId == mStreamId_warpMap) {
        MY_LOGD("find mStreamId_warpMap %d", mStreamId_warpMap);
        auto imgSize = capReq.inputBuffers[0].buffer->getImgSize();
        int64_t wpeOutW = imgSize.w, wpeOutH = imgSize.h;
        if (IMetadata::getEntry(&capReq.settings,
                     MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, wpeOutW, 0) != true)
          MY_LOGD("getEntry MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE Fail");
        if (IMetadata::getEntry(&capReq.settings,
                     MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, wpeOutH, 1) != true)
          MY_LOGD("getEntry MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE Fail");

        if (mUseWPEtoCrop) {
          writeWarpMap(buf, imgSize.w, imgSize.h, wpeOutW, wpeOutH);
        } else {
          writeWarpMap(buf, imgSize.w, imgSize.h);
        }
      }
      ++it;
    }
    // get output image buffer for captureRequest
    MY_LOGD("capReq.outputBuffers.size(%zu)", capReq.outputBuffers.size());
    for (StreamBuffer& buf : capReq.outputBuffers) {
      if (buf.buffer == nullptr) {
        AllocateBufferInfo bufInfo;
        if (getAllocateBufferInfo_RhsOutput(frameNo, buf.streamId, bufInfo)) {
          buf.buffer = bufInfo.pImgBufHeap;
          buf.bufferId = bufInfo.bufferId;
          MY_LOGD(
              "(repeat) RHS outStream: frameNo:%d, streamId:0x%X, buffer:%p, "
              "bufferId:%d",
              frameNo, buf.streamId, buf.buffer.get(), buf.bufferId);
        } else {
          MY_LOGA(
              "(repeat) RHS outStream: frameNo:%d, streamId:0x%X not in "
              "mRhsOutImgBufferInfos",
              frameNo, buf.streamId);
          return false;
        }
      } else {
        MY_LOGD(
            "(repeat) RHS outStream: frameNo:%d streamId:0x%X buffer is ready",
            frameNo, buf.streamId);
      }
    }
  }
  return true;
}

bool MtkHalData::allocateStreamBuffer() {
  mAllocateBufferSets.clear();
  for (int i = 0; i < mBufNum; i++) {
    AllocateBufferSet bufSet;
    // LHS output buffer
    for (auto& stream : lhsData.configSetting.streams) {
      AllocateBufferInfo bufInfo;
      if (!allocateIonBuffer(stream, &bufInfo)) {
        MY_LOGA("LHS allocateIonBuffer fail! (streamId:%d)", stream.id);
        return false;
      }
      bufSet.mLhsOutImgBufferInfos.emplace(stream.id, bufInfo);
      MY_LOGD("allocate buffer(0x%X) emplace to mLhsOutImgBufferInfos",
              stream.id);
    }
    // RHS input buffer
    for (auto& stream : rhsData.configSetting.streams) {
      if (stream.streamType == StreamType::INPUT) {
        // if RHS input buffer not found in LHS output buffer, need to allocate
        if (bufSet.mLhsOutImgBufferInfos.find(stream.id) ==
            bufSet.mLhsOutImgBufferInfos.end()) {
          AllocateBufferInfo bufInfo;
          if (!allocateIonBuffer(stream, &bufInfo)) {
            MY_LOGA("RHS allocateIonBuffer fail! (streamId:%d)", stream.id);
            return false;
          }
          bufSet.mRhsInImgBufferInfos.emplace(stream.id, bufInfo);
          MY_LOGD("allocate buffer(0x%X) emplace to mRhsInImgBufferInfos",
                  stream.id);
        } else {
          AllocateBufferInfo tmp = bufSet.mLhsOutImgBufferInfos.at(stream.id);
          bufSet.mRhsInImgBufferInfos.emplace(stream.id, tmp);
          MY_LOGD(
              "assign buffer(0x%X) from mLhsOutImgBufferInfos to "
              "mRhsInImgBufferInfos",
              stream.id);
        }
      }
    }
    // RHS output buffer
    for (auto& stream : rhsData.configSetting.streams) {
      if (stream.streamType == StreamType::OUTPUT) {
        AllocateBufferInfo bufInfo;
        if (!allocateIonBuffer(stream, &bufInfo)) {
          MY_LOGA("RHS allocateIonBuffer fail! (streamId:%d)", stream.id);
          return false;
        }
        bufSet.mRhsOutImgBufferInfos.emplace(stream.id, bufInfo);
        MY_LOGD("allocate buffer(0x%X) emplace to mRhsOutImgBufferInfos",
                stream.id);
      }
    }
    mAllocateBufferSets.emplace(i, bufSet);
  }
  return true;
}

bool MtkHalData::allocateIonBuffer(Stream& srcStream,
                                   AllocateBufferInfo* dstBufferInfo) {
  MY_LOGD("WillTest allocateIonBuffer : format : %d", srcStream.format);
  mcam::IImageBufferAllocator::ImgParam imgParam(
      srcStream.format, NSCam::MSize(srcStream.width, srcStream.height),
      srcStream.usage);
  if (srcStream.format == NSCam::EImageFormat::eImgFmt_BLOB ||
      srcStream.format == NSCam::EImageFormat::eImgFmt_JPEG) {
    mcam::IImageBufferAllocator::ImgParam jpegParam(
      srcStream.format, NSCam::MSize(srcStream.width*srcStream.height, 1),
      srcStream.usage);
    imgParam = jpegParam;
  }
  //
  if (srcStream.bufPlanes.planes[0].rowStrideInBytes == 0) {
    MY_LOGD("stride will be decided by ImageBuffer");
  } else {
    imgParam.strideInByte[0] = srcStream.bufPlanes.planes[0].rowStrideInBytes;
    imgParam.strideInByte[1] = srcStream.bufPlanes.planes[1].rowStrideInBytes;
    imgParam.strideInByte[2] = srcStream.bufPlanes.planes[2].rowStrideInBytes;
    MY_LOGD("ImageBuffer set stride [%d,%d,%d]", imgParam.strideInByte[0],
            imgParam.strideInByte[1], imgParam.strideInByte[2]);
  }
  //
  static int allocateBufSerialNo = 1;
  auto pImageBufferHeap = mcam::IImageBufferHeapFactory::create(
      LOG_TAG, "outStream", imgParam, true);
  dstBufferInfo->bufferId = allocateBufSerialNo++;
  dstBufferInfo->pImgBufHeap = pImageBufferHeap;
  //
  if (pImageBufferHeap == nullptr) {
    MY_LOGA("allocateIonBuffer fail, pImageBufferHeap is null!");
    return false;
  }
  return true;
}

#define GET_ALLOCATE_BUF_INFO(frameNo, streamId, src, bufInfo)        \
  int totalNum = mAllocateBufferSets.size();                          \
  int index = (frameNo % totalNum);                                   \
  if (mAllocateBufferSets.find(index) != mAllocateBufferSets.end()) { \
    AllocateBufferSet bufSet = mAllocateBufferSets.at(index);         \
    if (bufSet.m##src##ImgBufferInfos.find(streamId) !=               \
        bufSet.m##src##ImgBufferInfos.end()) {                        \
      bufInfo = bufSet.m##src##ImgBufferInfos.at(streamId);           \
    }                                                                 \
  } else {                                                            \
    MY_LOGE(                                                          \
        "totalNum(%d) frameNo(%d) index(%d) "                         \
        "mAllocateBufferSets.find(%d) fail!",                         \
        totalNum, frameNo, index);                                    \
    return false;                                                     \
  }                                                                   \
  return true;

bool MtkHalData::getAllocateBufferInfo_LhsOutput(int32_t frameNo,
                                                 int32_t streamId,
                                                 AllocateBufferInfo& bufInfo) {
  GET_ALLOCATE_BUF_INFO(frameNo, streamId, LhsOut, bufInfo);
}
bool MtkHalData::getAllocateBufferInfo_RhsInput(int32_t frameNo,
                                                int32_t streamId,
                                                AllocateBufferInfo& bufInfo) {
  GET_ALLOCATE_BUF_INFO(frameNo, streamId, RhsIn, bufInfo);
}
bool MtkHalData::getAllocateBufferInfo_RhsOutput(int32_t frameNo,
                                                 int32_t streamId,
                                                 AllocateBufferInfo& bufInfo) {
  GET_ALLOCATE_BUF_INFO(frameNo, streamId, RhsOut, bufInfo);
}

void MtkHalData::writeWarpMap(StreamBuffer& buf, int32_t imgW, int32_t imgH) {
  MY_LOGD("writeWarpMap start, imgW: %d, imgH: %d", imgW, imgH);
  // write warp map into buffer
  if (buf.buffer == nullptr)
    MY_LOGD("null buffer");
  auto& warpMapBuf = buf.buffer;
  warpMapBuf->lockBuf("WarpMap",
                      mcam::eBUFFER_USAGE_SW_READ_OFTEN |
                          mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                          mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE);
  int* pBufferVA_x = reinterpret_cast<int*>(warpMapBuf->getBufVA(0));
  int* pBufferVA_y = reinterpret_cast<int*>(warpMapBuf->getBufVA(1));

  // write identity map: (0,0) (w-1, 0) (0, h-1),  (w-1, h-1)
    int32_t shift[2] = {0};
    int32_t step[2] = {0};
    int32_t OutputW = imgW;
    int32_t OutputH = imgH;
    int32_t WarpMapW = 2;
    int32_t WarpMapH = 2;

    step[0] = OutputW / (WarpMapW - 1);
    step[1] = OutputH / (WarpMapH - 1);
    MY_LOGD("write identity map start");

    for (int j = 0; j < WarpMapH; j++) {
      for (int i = 0; i < WarpMapW; i++) {
        pBufferVA_x[j * WarpMapW + i] =
            (!shift[0] && !i) ? 0 : ((shift[0] + step[0] * i - 1) * 32);
        pBufferVA_y[j * WarpMapW + i] =
            (!shift[1] && !j) ? 0 : ((shift[1] + step[1] * j - 1) * 32);

        MY_LOGD("warp map k:%d, X value: %d, Y value: %d",
                j * WarpMapW + i, pBufferVA_x[j * WarpMapW + i],
                pBufferVA_y[j * WarpMapW + i]);
      }
    }

  warpMapBuf->unlockBuf("WarpMap");
}

void MtkHalData::writeWarpMap(StreamBuffer& buf, int32_t imgW, int32_t imgH,
                              int64_t wpeOutW, int64_t wpeOutH) {
  MY_LOGD("writeWarpMap start, imgW: %d, imgH: %d, wpeOutW: %d, wpeOutH: %d",
          imgW, imgH, wpeOutW, wpeOutH);
  // write warp map into buffer
  if (buf.buffer == nullptr)
    MY_LOGD("null buffer");
  auto& warpMapBuf = buf.buffer;
  warpMapBuf->lockBuf("WarpMap",
                      mcam::eBUFFER_USAGE_SW_READ_OFTEN |
                          mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                          mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE);
  int* pBufferVA_x = reinterpret_cast<int*>(warpMapBuf->getBufVA(0));
  int* pBufferVA_y = reinterpret_cast<int*>(warpMapBuf->getBufVA(1));

    struct WPE_Parameter {
      unsigned int in_width;
      unsigned int in_height;
      unsigned int in_offset_x;
      unsigned int in_offset_y;
      unsigned int out_width;
      unsigned int out_height;
      unsigned int warp_map_width;
      unsigned int warp_map_height;
    };

    WPE_Parameter wpe;
    wpe.in_width = imgW;
    wpe.in_height = imgH;
    wpe.in_offset_x = 0;
    wpe.in_offset_y = 0;
    wpe.out_width = wpeOutW;
    wpe.out_height = wpeOutH;
    wpe.warp_map_width = 2;
    wpe.warp_map_height = 2;

    if ((wpe.in_offset_x + wpe.out_width) > wpe.in_width) {
      MY_LOGF("Crop area is over input image width");
    } else if ((wpe.in_offset_y + wpe.out_height) > wpe.in_height) {
      MY_LOGF("Crop area is over input image height");
    }

    float stepX = static_cast<float>(wpe.out_width) /
                  static_cast<float>(wpe.warp_map_width - 1);
    float stepY = static_cast<float>(wpe.out_height) /
                  static_cast<float>(wpe.warp_map_height - 1);
    MY_LOGD("StepX = %f, StepY = %f", stepX, stepY);

    float oy = wpe.in_offset_y;
    float ox = 0;
    for (int i = 0; i < wpe.warp_map_height; i++) {
      ox = wpe.in_offset_x;
      for (int j = 0; j < wpe.warp_map_width; j++) {
        if (ox >= (wpe.in_offset_x + wpe.out_width))
          ox = wpe.in_offset_x + wpe.out_width - 1.0f;
        if (oy >= (wpe.in_offset_y + wpe.out_height))
          oy = wpe.in_offset_y + wpe.out_height - 1.0f;

        pBufferVA_x[i * wpe.warp_map_width + j] =
            static_cast<int>(ox * 32.0f);
        pBufferVA_y[i * wpe.warp_map_width + j] =
            static_cast<int>(oy * 32.0f);

        MY_LOGD("warp map k:%d, X value: %d, Y value: %d",
                i * wpe.warp_map_width + j,
                pBufferVA_x[i * wpe.warp_map_width + j],
                pBufferVA_y[i * wpe.warp_map_width + j]);

        ox += stepX;
      }
      oy += stepY;
    }

  warpMapBuf->unlockBuf("WarpMap");
}

bool convertMetaSettingToIMetadata(MetadataSetting const& in, IMetadata& out) {
#define UPDATE_META(_type)                                                  \
  if (in.tagValues.size() <= 0) {                                           \
    MY_LOGE("[0x%X][%s]tagValues is empty!", in.tagId, in.tagName.c_str()); \
    return false;                                                           \
  }                                                                         \
  if (in.tagValues.size() == 1) {                                           \
    err = IMetadata::setEntry<_type>(&out, in.tagId, in.tagValues[0]);      \
  } else {                                                                  \
    NSCam::IMetadata::IEntry entry(in.tagId);                               \
    for (auto& val : in.tagValues) {                                        \
      entry.push_back(val, NSCam::Type2Type<_type>());                      \
    }                                                                       \
    err = out.update(in.tagId, entry);                                      \
  }

  // MY_LOGD("convertMetaSettingToIMetadata: tagId:0x%X tagName:%s tagType:%s
  // tagValues[0]:%d", in.tagId, in.tagName.c_str(), in.tagType.c_str(),
  // in.tagValues[0]);
  int err = 0;
  if (in.tagType == "MUINT8") {
    UPDATE_META(MUINT8);
  } else if (in.tagType == "MINT32") {
    UPDATE_META(MINT32);
  } else if (in.tagType == "MINT64") {
    UPDATE_META(MINT64);
  } else if (in.tagType == "MFLOAT") {
    UPDATE_META(MFLOAT);
  } else if (in.tagType == "MDOUBLE") {
    UPDATE_META(MDOUBLE);
  } else if (in.tagType == "MPoint") {
    NSCam::IMetadata::IEntry entry(in.tagId);
    if (in.tagValues.size() != 2) {
      MY_LOGE("[0x%X][%s]type is [MPoint], but tagValues.size()!=2", in.tagId,
              in.tagName.c_str());
      return false;
    }
    MPoint tmp(in.tagValues[0], in.tagValues[1]);
    err = IMetadata::setEntry<MPoint>(&out, in.tagId, tmp);
  } else if (in.tagType == "MSize") {
    NSCam::IMetadata::IEntry entry(in.tagId);
    if (in.tagValues.size() != 2) {
      MY_LOGE("[0x%X][%s]type is [MPoint], but tagValues.size()!=2", in.tagId,
              in.tagName.c_str());
      return false;
    }
    MSize tmp(in.tagValues[0], in.tagValues[1]);
    err = IMetadata::setEntry<MSize>(&out, in.tagId, tmp);
  } else if (in.tagType == "MRect") {
    NSCam::IMetadata::IEntry entry(in.tagId);
    if (in.tagValues.size() != 4) {
      MY_LOGE("[0x%X][%s]type is [MRect], but tagValues.size()!=2", in.tagId,
              in.tagName.c_str());
      return false;
    }
    MRect tmp(MPoint(in.tagValues[0], in.tagValues[1]),
              MSize(in.tagValues[2], in.tagValues[3]));
    err = IMetadata::setEntry<MRect>(&out, in.tagId, tmp);
  } else {
    err = -1;
    MY_LOGE("Unsupport Tag Type:%s !", in.tagType.c_str());
  }
  return (err == 0) ? true : false;
}

bool MtkHalData::transformFromJsonData(JsonData const& jsonData,
                                       IMetadata& templateMeta) {
  // basic info
  mStreamId_ME = jsonData.streamId_ME;
  mStreamId_TME = jsonData.streamId_TME;
  mStreamId_warpMap = jsonData.streamId_warpMap;
  mUseWPEtoCrop = jsonData.useWPEtoCrop;
  // LHS config setting
  lhsData.configSetting.operationMode =
      (StreamConfigurationMode)jsonData.operationMode;
  lhsData.configSetting.streamConfigCounter = 0;
  // LHS config setting - outputStreams
  for (auto const& s : jsonData.Lhs.configSetting.outputStreams) {
    Stream tmpStream;
    tmpStream.id = s.streamId;
    tmpStream.streamType = StreamType::OUTPUT;
    tmpStream.width = s.width;
    tmpStream.height = s.height;
    tmpStream.format = (EImageFormat)s.format;
    calcuBufPlane(s.width, s.height, s.stride, (EImageFormat)s.format,
                  tmpStream.bufPlanes);
    tmpStream.usage = s.usage;
    tmpStream.dataSpace = (Dataspace)s.dataSpace;
    tmpStream.transform = s.transform;
    tmpStream.physicalCameraId = s.physicalCameraId;
    for (auto const& m : s.settings) {
      if (!convertMetaSettingToIMetadata(m, tmpStream.settings)) {
        MY_LOGE("convertMetaSettingToIMetadata Fail!");
        return false;
      }
    }
    lhsData.configSetting.streams.push_back(tmpStream);
  }
  // LHS config setting - sessionParams
  for (auto const& m : jsonData.Lhs.configSetting.sessionParams) {
    if (!convertMetaSettingToIMetadata(m,
                                       lhsData.configSetting.sessionParams)) {
      MY_LOGE("convertMetaSettingToIMetadata Fail!");
      return false;
    }
  }
  // Final Session Params = template Setting + JSon Setting
  {
    IMetadata oriSessionParams = lhsData.configSetting.sessionParams;
    lhsData.configSetting.sessionParams =
        templateMeta + lhsData.configSetting.sessionParams;
    MY_LOGD("sessionParams.size(%d) + templateMeta.size(%d) = final.size(%d)",
            oriSessionParams.count(), templateMeta.count(),
            lhsData.configSetting.sessionParams.count());
  }
  // LHS request setting
  bool bFoundRepeat = false;
  for (auto const& r : jsonData.Lhs.requestSettings) {
    if (r.frameNo.size() == 1 && r.frameNo[0] == -1) {
      if (bFoundRepeat == true) {
        MY_LOGE("Only allow 1 repeat request setting");
        return false;
      }
      bFoundRepeat = true;
    }
    //
    if (bFoundRepeat) {
      // need to modified frameNo later
      lhsData.repeatRequest.frameNumber = 0;
      for (auto const& stream : r.outputStreams) {
        StreamBuffer streamBuf;
        streamBuf.streamId = stream.streamId;
        //
        for (auto const& meta : stream.streamSettings) {
          if (!convertMetaSettingToIMetadata(meta, streamBuf.bufferSettings)) {
            MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
            return false;
          }
        }
        lhsData.repeatRequest.outputBuffers.push_back(streamBuf);
      }
      for (auto const& meta : r.appMetadatas) {
        if (!convertMetaSettingToIMetadata(meta,
                                           lhsData.repeatRequest.settings)) {
          MY_LOGE("convertMetaSettingToIMetadata (appMetadatas) Fail!");
          return false;
        }
      }
      for (auto const& meta : r.halMetadatas) {
        if (!convertMetaSettingToIMetadata(meta,
                                           lhsData.repeatRequest.halSettings)) {
          MY_LOGE("convertMetaSettingToIMetadata (halMetadatas) Fail!");
          return false;
        }
      }
      // Final Request AppMetadata = template Setting + JSon Setting
      IMetadata oriAppSetting = lhsData.repeatRequest.settings;
      lhsData.repeatRequest.settings =
          templateMeta + lhsData.repeatRequest.settings;
      MY_LOGD("appMetadata.size(%d) + templateMeta.size(%d) = final.size(%d)",
              oriAppSetting.count(), templateMeta.count(),
              lhsData.repeatRequest.settings.count());

    } else {
      for (auto const& f : r.frameNo) {
        CaptureRequest capReq;
        capReq.frameNumber = f;
        for (auto const& stream : r.outputStreams) {
          StreamBuffer streamBuf;
          streamBuf.streamId = stream.streamId;
          //
          for (auto const& meta : stream.streamSettings) {
            if (!convertMetaSettingToIMetadata(meta,
                                               streamBuf.bufferSettings)) {
              MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
              return false;
            }
          }
          capReq.outputBuffers.push_back(streamBuf);
        }
        for (auto const& meta : r.appMetadatas) {
          if (!convertMetaSettingToIMetadata(meta, capReq.settings)) {
            MY_LOGE("convertMetaSettingToIMetadata (appMetadatas) Fail!");
            return false;
          }
        }
        for (auto const& meta : r.halMetadatas) {
          if (!convertMetaSettingToIMetadata(meta, capReq.halSettings)) {
            MY_LOGE("convertMetaSettingToIMetadata (halMetadatas) Fail!");
            return false;
          }
        }
        // Final Request AppMetadata = template Setting + JSon Setting
        IMetadata oriAppSetting = capReq.settings;
        capReq.settings = templateMeta + capReq.settings;
        MY_LOGD("appMetadata.size(%d) + templateMeta.size(%d) = final.size(%d)",
                oriAppSetting.count(), templateMeta.count(),
                capReq.settings.count());
        //
        lhsData.specificRequestsMap.emplace(f, capReq);
      }
    }
  }

  //////////////////////////////////////////

  // RHS config setting
  rhsData.configSetting.operationMode =
      (StreamConfigurationMode)jsonData.operationMode;
  rhsData.configSetting.streamConfigCounter = 0;
  // RHS config setting - inputStreams
  for (auto const& s : jsonData.Rhs.configSetting.inputStreams) {
    Stream tmpStream;
    tmpStream.id = s.streamId;
    tmpStream.streamType = StreamType::INPUT;
    tmpStream.width = s.width;
    tmpStream.height = s.height;
    tmpStream.format = (EImageFormat)s.format;
    calcuBufPlane(s.width, s.height, s.stride, (EImageFormat)s.format,
                  tmpStream.bufPlanes);
    tmpStream.usage = s.usage;
    tmpStream.dataSpace = (Dataspace)s.dataSpace;
    tmpStream.transform = s.transform;
    tmpStream.physicalCameraId = s.physicalCameraId;
    for (auto const& m : s.settings) {
      if (!convertMetaSettingToIMetadata(m, tmpStream.settings)) {
        MY_LOGE("convertMetaSettingToIMetadata Fail!");
        return false;
      }
    }
    rhsData.configSetting.streams.push_back(tmpStream);
  }
  // RHS config setting - outputStreams
  for (auto const& s : jsonData.Rhs.configSetting.outputStreams) {
    Stream tmpStream;
    tmpStream.id = s.streamId;
    tmpStream.streamType = StreamType::OUTPUT;
    tmpStream.width = s.width;
    tmpStream.height = s.height;
    tmpStream.format = (EImageFormat)s.format;
    calcuBufPlane(s.width, s.height, s.stride, (EImageFormat)s.format,
                  tmpStream.bufPlanes);
    tmpStream.usage = s.usage;
    tmpStream.dataSpace = (Dataspace)s.dataSpace;
    tmpStream.transform = s.transform;
    tmpStream.physicalCameraId = s.physicalCameraId;
    for (auto const& m : s.settings) {
      if (!convertMetaSettingToIMetadata(m, tmpStream.settings)) {
        MY_LOGE("convertMetaSettingToIMetadata Fail!");
        return false;
      }
    }
    rhsData.configSetting.streams.push_back(tmpStream);
  }
  // RHS config setting - sessionParams
  for (auto const& m : jsonData.Rhs.configSetting.sessionParams) {
    if (!convertMetaSettingToIMetadata(m,
                                       rhsData.configSetting.sessionParams)) {
      MY_LOGE("convertMetaSettingToIMetadata Fail!");
      return false;
    }
  }
  // Final Session Params = template Setting + JSon Setting
  {
    IMetadata oriSessionParams = rhsData.configSetting.sessionParams;
    rhsData.configSetting.sessionParams =
        templateMeta + rhsData.configSetting.sessionParams;
    MY_LOGD("sessionParams.size(%d) + templateMeta.size(%d) = final.size(%d)",
            oriSessionParams.count(), templateMeta.count(),
            rhsData.configSetting.sessionParams.count());
  }
  // RHS request setting
  bFoundRepeat = false;
  for (auto const& r : jsonData.Rhs.requestSettings) {
    if (r.frameNo.size() == 1 && r.frameNo[0] == -1) {
      if (bFoundRepeat == true) {
        MY_LOGE("Only allow 1 repeat request setting");
        return false;
      }
      bFoundRepeat = true;
    }
    // Repeat request
    if (bFoundRepeat) {
      // need to modified frameNo later
      rhsData.repeatRequest.frameNumber = 0;
      // inputStreams
      for (auto const& stream : r.inputStreams) {
        StreamBuffer streamBuf;
        streamBuf.streamId = stream.streamId;
        //
        for (auto const& meta : stream.streamSettings) {
          if (!convertMetaSettingToIMetadata(meta, streamBuf.bufferSettings)) {
            MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
            return false;
          }
        }
        rhsData.repeatRequest.inputBuffers.push_back(streamBuf);
      }
      // outputStreams
      for (auto const& stream : r.outputStreams) {
        StreamBuffer streamBuf;
        streamBuf.streamId = stream.streamId;
        //
        for (auto const& meta : stream.streamSettings) {
          if (!convertMetaSettingToIMetadata(meta, streamBuf.bufferSettings)) {
            MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
            return false;
          }
        }
        rhsData.repeatRequest.outputBuffers.push_back(streamBuf);
      }
      // appMetadatas
      for (auto const& meta : r.appMetadatas) {
        if (!convertMetaSettingToIMetadata(meta,
                                           rhsData.repeatRequest.settings)) {
          MY_LOGE("convertMetaSettingToIMetadata (appMetadatas) Fail!");
          return false;
        }
      }
      // halMetadatas
      for (auto const& meta : r.halMetadatas) {
        if (!convertMetaSettingToIMetadata(meta,
                                           rhsData.repeatRequest.halSettings)) {
          MY_LOGE("convertMetaSettingToIMetadata (halMetadatas) Fail!");
          return false;
        }
      }
      // Final Request AppMetadata = template Setting + JSon Setting
      IMetadata oriAppSetting = rhsData.repeatRequest.settings;
      rhsData.repeatRequest.settings =
          templateMeta + rhsData.repeatRequest.settings;
      MY_LOGD("appMetadata.size(%d) + templateMeta.size(%d) = final.size(%d)",
              oriAppSetting.count(), templateMeta.count(),
              rhsData.repeatRequest.settings.count());
    } else {
      // specific request
      for (auto const& f : r.frameNo) {
        CaptureRequest capReq;
        capReq.frameNumber = f;
        // inputStreams
        for (auto const& stream : r.inputStreams) {
          StreamBuffer streamBuf;
          streamBuf.streamId = stream.streamId;
          //
          for (auto const& meta : stream.streamSettings) {
            if (!convertMetaSettingToIMetadata(meta,
                                               streamBuf.bufferSettings)) {
              MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
              return false;
            }
          }
          capReq.inputBuffers.push_back(streamBuf);
        }
        // outputStreams
        for (auto const& stream : r.outputStreams) {
          StreamBuffer streamBuf;
          streamBuf.streamId = stream.streamId;
          //
          for (auto const& meta : stream.streamSettings) {
            if (!convertMetaSettingToIMetadata(meta,
                                               streamBuf.bufferSettings)) {
              MY_LOGE("convertMetaSettingToIMetadata (stream Metadatas) Fail!");
              return false;
            }
          }
          capReq.outputBuffers.push_back(streamBuf);
        }
        // appMetadatas
        for (auto const& meta : r.appMetadatas) {
          if (!convertMetaSettingToIMetadata(meta, capReq.settings)) {
            MY_LOGE("convertMetaSettingToIMetadata (appMetadatas) Fail!");
            return false;
          }
        }
        // halMetadatas
        for (auto const& meta : r.halMetadatas) {
          if (!convertMetaSettingToIMetadata(meta, capReq.halSettings)) {
            MY_LOGE("convertMetaSettingToIMetadata (halMetadatas) Fail!");
            return false;
          }
        }
        // Final Request AppMetadata = template Setting + JSon Setting
        IMetadata oriAppSetting = capReq.settings;
        capReq.settings = templateMeta + capReq.settings;
        MY_LOGD("appMetadata.size(%d) + templateMeta.size(%d) = final.size(%d)",
                oriAppSetting.count(), templateMeta.count(),
                capReq.settings.count());
        //
        rhsData.specificRequestsMap.emplace(f, capReq);
      }
    }
  }
  //////////////////////////////////////////

  mbTransformed = true;
  MY_LOGD("TransformData_Json2MtkHal() successfully!");
  return true;
}

JSonUtil::ParseResult JSonUtil::ParseJsonFile(const char* filePath,
                                              JsonData& jsonData) {
  Json::Reader reader;
  Json::Value root;
  ///////////////////////////////////////////////
  // 1. open and read json file
  std::ifstream is;
  is.open(filePath, std::ios::binary);
  if (!reader.parse(is, root)) {
    is.close();
    MY_LOGE("reader.parse : %s failed!", filePath);
    MY_LOGE("error msg: %s", reader.getFormatedErrorMessages().c_str());
    return OPEN_FILE_FAILED;
  }
  jsonData.filePath = filePath;
  ///////////////////////////////////////////////
  // 2. check the key word for the json file
  jsonData.keyName = "MTK_HAL_UT_TEST";
  if (!root.isMember(jsonData.keyName)) {
    MY_LOGE("keyName:%s not found!", jsonData.keyName.c_str());
    return KEY_NAME_NOT_MATCH;
  }
  ///////////////////////////////////////////////
  // 3. start to parse json content
  //    json file is composed of the following part:
  //
  //    (3.1)basic info
  //    (3.2)lhs part
  //        (3.2.1)lhs config - outputstreams
  //        (3.2.2)lhs config - session params
  //        (3.2.3)lhs request
  //    (3.3)rhs part
  //        (3.3.1)rhs config - inputstreams
  //        (3.3.2)rhs config - outputstreams
  //        (3.3.3)rhs config - session params
  //        (3.3.4)rhs request

  // (3.1)basic info
  root = root[jsonData.keyName];
  CHECK_AND_GET_STRING(root, "caseName", jsonData.caseName);
  CHECK_AND_GET_STRING(root, "caseType", jsonData.caseType);
  CHECK_AND_GET_INT(root, "cameraDeviceId", jsonData.cameraDeviceId);
  CHECK_AND_GET_INT(root, "OperationMode", jsonData.operationMode);
  TRY_TO_GET_INT(root, "rhsDeviceId", jsonData.rhsDeviceId);
  TRY_TO_GET_INT(root, "ME_StreamID", jsonData.streamId_ME);
  TRY_TO_GET_INT(root, "TME_StreamID", jsonData.streamId_TME);
  TRY_TO_GET_INT(root, "warpMap_StreamID", jsonData.streamId_warpMap);
  TRY_TO_GET_INT(root, "useWPEtoCrop", jsonData.useWPEtoCrop);
  // (3.2)lhs part
  CHECK_IS_MEMBER(root, "LHS");
  Json::Value rootLhs = root["LHS"];
  CHECK_IS_MEMBER(rootLhs, "config");
  Json::Value rootLhsCfg = rootLhs["config"];

  // (3.2.1)lhs config - outputstreams
  CHECK_IS_MEMBER(rootLhsCfg, "outputStreams");
  Json::Value lhsCfgOutputStreams = rootLhsCfg["outputStreams"];
  for (auto& s : lhsCfgOutputStreams) {
    StreamSetting stream;
    CHECK_AND_GET_INT(s, "streamId", stream.streamId);
    CHECK_AND_GET_INT(s, "format", stream.format);
    CHECK_AND_GET_INT(s, "width", stream.width);
    CHECK_AND_GET_INT(s, "height", stream.height);
    CHECK_AND_GET_INT(s, "stride", stream.stride);
    CHECK_AND_GET_INT(s, "usage", stream.usage);
    CHECK_AND_GET_INT(s, "dataSpace", stream.dataSpace);
    CHECK_AND_GET_INT(s, "transform", stream.transform);
    CHECK_AND_GET_INT(s, "physicalCameraId", stream.physicalCameraId);
    //
    Json::Value lhsCfgOutStreamMeta = s["settings"];
    for (auto& m : lhsCfgOutStreamMeta) {
      MetadataSetting meta;
      CHECK_AND_GET_METADATA(m, meta);
      stream.settings.push_back(meta);
    }
    jsonData.Lhs.configSetting.outputStreams.push_back(stream);
  }
  // (3.2.2)lhs config - session params
  Json::Value lhsCfgSessionParams = rootLhsCfg["sessionParams"];
  for (auto& m : lhsCfgSessionParams) {
    MetadataSetting meta;
    CHECK_AND_GET_METADATA(m, meta);
    jsonData.Lhs.configSetting.sessionParams.push_back(meta);
  }
  // (3.2.3)lhs request
  CHECK_IS_MEMBER(rootLhs, "requests");
  Json::Value lhsRequests = rootLhs["requests"];
  for (auto& r : lhsRequests) {
    RequestSetting req;
    CHECK_AND_GET_DIGITAL_NUMBER_ARRAY(r, "frameNo", req.frameNo);
    //
    for (auto& s : r["outputStreams"]) {
      RequestStream stream;
      CHECK_AND_GET_INT(s, "streamId", stream.streamId);
      //
      Json::Value lhsReqOutStreamMeta = s["settings"];
      for (auto& mm : lhsReqOutStreamMeta) {
        MetadataSetting meta;
        CHECK_AND_GET_METADATA(mm, meta);
        stream.streamSettings.push_back(meta);
      }
      req.outputStreams.push_back(stream);
    }
    //
    CHECK_IS_MEMBER(r, "app_metadata");
    Json::Value appMetas = r["app_metadata"];
    for (auto& m : appMetas) {
      MetadataSetting meta;
      CHECK_AND_GET_METADATA(m, meta);
      req.appMetadatas.push_back(meta);
    }
    //
    CHECK_IS_MEMBER(r, "hal_metadata");
    Json::Value halMetas = r["hal_metadata"];
    for (auto& m : halMetas) {
      MetadataSetting meta;
      CHECK_AND_GET_METADATA(m, meta);
      req.halMetadatas.push_back(meta);
    }
    jsonData.Lhs.requestSettings.push_back(req);
  }
  // (3.3)rhs part
  if (root.isMember("RHS")) {
    Json::Value rootRhs = root["RHS"];
    CHECK_IS_MEMBER(rootRhs, "config");
    Json::Value rootRhsCfg = rootRhs["config"];
    // (3.3.1)rhs config - inputstreams
    CHECK_IS_MEMBER(rootRhsCfg, "inputStreams");
    Json::Value rhsCfgInputStreams = rootRhsCfg["inputStreams"];
    for (auto& s : rhsCfgInputStreams) {
      StreamSetting stream;
      CHECK_AND_GET_INT(s, "streamId", stream.streamId);
      CHECK_AND_GET_INT(s, "format", stream.format);
      CHECK_AND_GET_INT(s, "width", stream.width);
      CHECK_AND_GET_INT(s, "height", stream.height);
      CHECK_AND_GET_INT(s, "stride", stream.stride);
      CHECK_AND_GET_INT(s, "usage", stream.usage);
      //
      Json::Value rhsCfgInStreamMeta = s["settings"];
      for (auto& m : rhsCfgInStreamMeta) {
        MetadataSetting meta;
        CHECK_AND_GET_METADATA(m, meta);
        stream.settings.push_back(meta);
      }
      jsonData.Rhs.configSetting.inputStreams.push_back(stream);
    }
    // (3.3.2)rhs config - outputstreams
    CHECK_IS_MEMBER(rootRhsCfg, "outputStreams");
    Json::Value rhsCfgOutputStreams = rootRhsCfg["outputStreams"];
    for (auto& s : rhsCfgOutputStreams) {
      StreamSetting stream;
      CHECK_AND_GET_INT(s, "streamId", stream.streamId);
      CHECK_AND_GET_INT(s, "format", stream.format);
      CHECK_AND_GET_INT(s, "width", stream.width);
      CHECK_AND_GET_INT(s, "height", stream.height);
      CHECK_AND_GET_INT(s, "stride", stream.stride);
      CHECK_AND_GET_INT(s, "usage", stream.usage);
      CHECK_AND_GET_INT(s, "dataSpace", stream.dataSpace);
      CHECK_AND_GET_INT(s, "transform", stream.transform);
      CHECK_AND_GET_INT(s, "physicalCameraId", stream.physicalCameraId);
      //
      Json::Value rhsCfgOutStreamMeta = s["settings"];
      for (auto& m : rhsCfgOutStreamMeta) {
        MetadataSetting meta;
        CHECK_AND_GET_METADATA(m, meta);
        stream.settings.push_back(meta);
      }
      jsonData.Rhs.configSetting.outputStreams.push_back(stream);
    }
    // (3.3.3)rhs config - session params
    Json::Value rhsCfgSessionParams = rootRhsCfg["sessionParams"];
    for (auto& m : rhsCfgSessionParams) {
      MetadataSetting meta;
      CHECK_AND_GET_METADATA(m, meta);
      jsonData.Rhs.configSetting.sessionParams.push_back(meta);
    }
    // (3.3.4)rhs request
    CHECK_IS_MEMBER(rootRhs, "requests");
    Json::Value rhsRequests = rootRhs["requests"];

    for (auto& r : rhsRequests) {
      RequestSetting req;
      CHECK_AND_GET_DIGITAL_NUMBER_ARRAY(r, "frameNo", req.frameNo);
      //
      for (auto& s : r["inputStreams"]) {
        RequestStream stream;
        CHECK_AND_GET_INT(s, "streamId", stream.streamId);
        //
        Json::Value rhsReqInStreamMeta = s["settings"];
        for (auto& m : rhsReqInStreamMeta) {
          MetadataSetting meta;
          CHECK_AND_GET_METADATA(m, meta);
          stream.streamSettings.push_back(meta);
        }
        req.inputStreams.push_back(stream);
      }
      for (auto& s : r["outputStreams"]) {
        RequestStream stream;
        CHECK_AND_GET_INT(s, "streamId", stream.streamId);
        //
        Json::Value rhsReqOutStreamMeta = s["settings"];
        for (auto& m : rhsReqOutStreamMeta) {
          MetadataSetting meta;
          CHECK_AND_GET_METADATA(m, meta);
          stream.streamSettings.push_back(meta);
        }
        req.outputStreams.push_back(stream);
      }
      //
      CHECK_IS_MEMBER(r, "app_metadata");
      Json::Value appMetas = r["app_metadata"];
      for (auto& m : appMetas) {
        MetadataSetting meta;
        CHECK_AND_GET_METADATA(m, meta);
        req.appMetadatas.push_back(meta);
      }
      //
      CHECK_IS_MEMBER(r, "hal_metadata");
      Json::Value halMetas = r["hal_metadata"];
      for (auto& m : halMetas) {
        MetadataSetting meta;
        CHECK_AND_GET_METADATA(m, meta);
        req.halMetadatas.push_back(meta);
      }
      jsonData.Rhs.requestSettings.push_back(req);
    }
  }
  ///////////////////////////////////////////////
  // 4. close json file
  is.close();
  return SUCCESSFUL;
}

JsonParser::JsonParser() : mDebugLevel(getDebugLevel()) {}

JsonParser::~JsonParser() {}

int JsonParser::parseJson(bool const& bEnableNativeDevice,
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
                          int32_t& meStreamId) {
  FUNC_START
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

  struct config lhsStreamConfigStrings;
  struct config rhsStreamConfigStrings;
  std::vector<struct request> lhsRequestStrings;
  std::vector<struct request> rhsRequestStrings;

  fd.open(mJSONFilePath.c_str(), std::ios::in);
  if (fd.is_open()) {
    while (std::getline(fd, str)) {
      strVec.push_back(str);
      lineNum++;
      fileStr.emplace(lineNum, str);
      //
      MY_LOGD_IF(mDebugLevel > 1, "lineNum(%04d) fileStr:%s", lineNum,
                 str.c_str());
      //
      if (str.find("cameraId") != std::string::npos) {
        auto start = str.find(": \"");
        auto end = str.find("\",");
        if (start != end && start != std::string::npos &&
            end != std::string::npos) {
          openId = str[start + 3] - '0';
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
        if (beg != std::string::npos && end != std::string::npos &&
            beg != end) {
          deviceType = str.substr(beg + 4, (end - beg - 4));
          MY_LOGI("deviceType:%s", deviceType.c_str());
        }
      }

      if (str.find("config") != std::string::npos) {
        if (hal3StartLine != 0 && lineNum > hal3StartLine &&
            ispStartLine == 0) {
          hal3ConfigLine = lineNum;
        }
        if (ispStartLine != 0 && lineNum > ispStartLine) {
          ispConfigLine = lineNum;
        }
      }

      if (str.find("requests") != std::string::npos) {
        if (hal3StartLine != 0 && lineNum > hal3StartLine &&
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
            MY_LOGI_IF(mDebugLevel > 1, "pairMap: {:(%d) }:(%d) ", tmp->first,
                       iter->first);
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
        MY_LOGI_IF(mDebugLevel > 1, "pairMap: {:(%d) }:(%d) ", iter->first,
                   iter->first);
        iter->second = " ";
      }
    }

    auto parseStram = [&](std::vector<struct pairSymbol>::iterator iter,
                          std::string str, std::string type) mutable {
      auto startline = iter->beg;
      auto endline = iter->end;
      struct config* configType = nullptr;
      std::vector<std::string>* tmp = nullptr;
      if (startline == endline) {
        MY_LOGW("nothing for metadata");
      } else {
        if (type == "hal3") {
          configType = &lhsStreamConfigStrings;
        }
        if (type == "isp") {
          configType = &rhsStreamConfigStrings;
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

        for (auto line = startline; line < (endline - 1); line++) {
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
              strVec[tmp->beg - 1].find("metadata") != std::string::npos) {
            parseStram(tmp, "metadata", "hal3");
          }
          if (tmp->beg > iter->beg &&
              strVec[tmp->beg - 1].find("outputStreams") != std::string::npos) {
            parseStram(tmp, "outputStreams", "hal3");
          }
          if (tmp->beg > iter->beg &&
              strVec[tmp->beg - 1].find("inputStreams") != std::string::npos) {
            parseStram(tmp, "inputStreams", "hal3");
          }
        }
      }

      if (iter->beg == ispConfigLine) {
        for (; tmp != pairVec.begin(); tmp--) {
          if (tmp->beg > iter->beg &&
              strVec[tmp->beg - 1].find("metadata") != std::string::npos) {
            parseStram(tmp, "metadata", "isp");
          }
          if (tmp->beg > iter->beg &&
              strVec[tmp->beg - 1].find("outputStreams") != std::string::npos) {
            parseStram(tmp, "outputStreams", "isp");
          }
          if (tmp->beg > iter->beg &&
              strVec[tmp->beg - 1].find("inputStreams") != std::string::npos) {
            parseStram(tmp, "inputStreams", "isp");
          }
        }
      }
    }
    for (auto iter = pairMap.begin(); iter != pairMap.end(); iter++) {
      // parse hal3 request
      auto parseRequest = [&](std::map<int, int>::iterator it,
                              std::string str) {
        // int reqNum =0;
        std::map<int, int> reqMap;
        auto tmp = it;
        tmp++;
        int start = tmp->first;
        int end = tmp->second;
        reqMap.emplace(start, end);
        for (auto iter = tmp; iter != pairMap.end(); iter++) {
          if (iter->first > tmp->second && iter->second < it->second) {
            reqMap.emplace(iter->first, iter->second);
            tmp = iter;
          }
        }
        for (auto vec : reqMap) {
          MY_LOGI_IF(mDebugLevel > 1, "start:%d, end:%d", vec.first,
                     vec.second);
        }
        for (auto vec : reqMap) {
          struct request reqTmp;
          for (int line = vec.first; line < vec.second; line++) {
            reqTmp.info.push_back(fileStr[line]);
            MY_LOGI_IF(mDebugLevel > 1, "reqTmp.info.push_back: %s",
                       fileStr[line].c_str());
          }
          if (str == "hal3")
            lhsRequestStrings.push_back(reqTmp);
          if (str == "isp")
            rhsRequestStrings.push_back(reqTmp);
        }
      };

      if (iter->first == hal3RequestLine) {
        parseRequest(iter, "hal3");
      }
      if (iter->first == ispRequestLine) {
        parseRequest(iter, "isp");
      }
      MY_LOGI("lhsRequestStrings size:%lu, rhsRequestStrings size:%lu",
              lhsRequestStrings.size(), rhsRequestStrings.size());
    }
    fd.close();
  } else {
    MY_LOGE("cannot open the config file");
    return 1;
  }
  //////////////////////////////////////////
  auto getSubStr_vec = [](std::string str, std::string beg, std::string medium,
                          std::string end, std::vector<int32_t>* vec) {
    auto start = str.find(beg);
    auto last = str.find(end);
    auto mapToValue = [&](std::string str) {
      int i32 = std::stoi(str);
      vec->push_back(i32);
    };
    if (start != std::string::npos && last != std::string::npos &&
        start != last) {
      auto med = str.find(medium);
      while (med != std::string::npos && med < last) {
        auto subStr = str.substr(start + 1, (med - start - 1));
        mapToValue(subStr);
        start = med + 1;
        str[med] = ' ';
        med = str.find(",");
      }
    }
    auto subStr = str.substr(start + 1, (last - start - 1));
    mapToValue(subStr);
  };
  //

  auto parseInfo = [&](std::vector<std::string> srcVec,
                       std::vector<Stream>& outStream) {
    int num = 0;
    for (auto str : srcVec) {
      if (str.find("id") != std::string::npos) {
        num++;
      }
    }
    //
    auto getSubStr = [&](std::string& src, std::string name,
                         std::string delimiter, int len, std::string& subStr) {
      subStr.clear();
      auto start = src.find(name);
      auto end = src.find(delimiter);
      if (start != std::string::npos && end != std::string::npos &&
          start != end) {
        subStr = src.substr(start + len, (end - start - len));
        src[end] = ' ';
      }
    };
    //
    auto calcuBufPlane = [](int32_t width, int32_t height, int32_t stride,
                            int32_t format, NSCam::BufPlanes& bufPlanes) {
#define addBufPlane(idx, planes, height, stride) \
  do {                                           \
    planes[idx] = {                              \
        .sizeInBytes = (height * stride),        \
        .rowStrideInBytes = stride,              \
    };                                           \
  } while (0)
      switch (format) {
        case NSCam::EImageFormat::eImgFmt_BLOB:
        case NSCam::EImageFormat::eImgFmt_JPEG:
          bufPlanes.count = 1;
          addBufPlane(0, bufPlanes.planes, 1, (size_t)stride);
          break;
        case NSCam::EImageFormat::eImgFmt_YUY2:  // direct yuv
        case NSCam::EImageFormat::eImgFmt_BAYER10:
        case NSCam::EImageFormat::eImgFmt_BAYER12:
        case NSCam::EImageFormat::eImgFmt_BAYER10_UNPAK:
        case NSCam::EImageFormat::eImgFmt_BAYER12_UNPAK:
        case NSCam::EImageFormat::eImgFmt_FG_BAYER10:
        case NSCam::EImageFormat::eImgFmt_FG_BAYER12:
        case NSCam::EImageFormat::eImgFmt_BAYER8:  // LCSO
        case NSCam::EImageFormat::eImgFmt_STA_BYTE:
        case NSCam::EImageFormat::eImgFmt_STA_2BYTE:  // LCSO with LCE3.0
        case NSCam::EImageFormat::eImgFmt_BAYER16_APPLY_LSC:  // for AF input
                                                              // raw in ISP6.0
        case NSCam::EImageFormat::eImgFmt_Y8:
          bufPlanes.count = 1;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          break;
        case NSCam::EImageFormat::eImgFmt_NV16:  // direct yuv 2 plane
          bufPlanes.count = 2;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          addBufPlane(1, bufPlanes.planes, height, (size_t)stride);
          break;
        case NSCam::EImageFormat::eImgFmt_MTK_YUV_P010:  // direct yuv 10bit, 2
                                                         // plane
        case NSCam::EImageFormat::eImgFmt_MTK_YUV_P210:
        case NSCam::EImageFormat::eImgFmt_MTK_YUV_P012:
        case NSCam::EImageFormat::eImgFmt_YUV_P010:  // direct yuv 10bit, 2
                                                     // plane
        case NSCam::EImageFormat::eImgFmt_YUV_P210:
        case NSCam::EImageFormat::eImgFmt_YUV_P012:
        case NSCam::EImageFormat::eImgFmt_NV21:
        case NSCam::EImageFormat::eImgFmt_NV12:
          bufPlanes.count = 2;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          addBufPlane(1, bufPlanes.planes, (height >> 1), (size_t)stride);
          break;
        case NSCam::EImageFormat::eImgFmt_YV12:
          bufPlanes.count = 3;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          addBufPlane(1, bufPlanes.planes, (height >> 1), (size_t)stride >> 1);
          addBufPlane(2, bufPlanes.planes, (height >> 1), (size_t)stride >> 1);
          break;
        case NSCam::EImageFormat::eImgFmt_WARP_2PLANE:
          bufPlanes.count = 2;
          addBufPlane(0, bufPlanes.planes, height, (size_t)stride);
          addBufPlane(1, bufPlanes.planes, height, (size_t)stride);
          break;
        default:
          MY_LOGA("unsupported format:%d - imgSize:%dx%d stride:%zu", format,
                  width, height, (size_t)stride);
          break;
      }
#undef addBufPlane
    };

    auto checkStrideAlign = [](int32_t stride, int32_t format) {
      switch (format) {
        case NSCam::EImageFormat::eImgFmt_MTK_YUV_P010:
          if (stride % 80 != 0)
            MY_LOGF("stride(%d) is not 80 bytes align for P010", stride);
          break;
        case NSCam::EImageFormat::eImgFmt_MTK_YUV_P012:
          if (stride % 96 != 0)
            MY_LOGF("stride(%d) is not 96 bytes align for P012", stride);
          break;
        case NSCam::EImageFormat::eImgFmt_NV21:
          if (stride % 64 != 0)
            MY_LOGF("stride(%d) is not 64 bytes align for NV21", stride);
          break;
        default:
          if (stride % 4 != 0)
            MY_LOGF("stride(%d) is not 4 bytes align", stride);
          break;
      }
    };
    //
    for (auto vec : srcVec) {
      MY_LOGI_IF(mDebugLevel > 1, "vec:%s", vec.c_str());
      int32_t streamId = 0;
      std::string tmpStr;
      getSubStr(vec, "\"id\"", ", ", 5, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      streamId = std::stoi(tmpStr);
      //
      getSubStr(vec, "\"format\"", ", ", 9, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto format = std::stoi(tmpStr);
      //
      getSubStr(vec, "\"width\"", ", ", 8, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto width = std::stoi(tmpStr);
      //
      getSubStr(vec, "\"height\"", ", ", 9, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto height = std::stoi(tmpStr);
      //
      getSubStr(vec, "\"stride\"", ",", 9, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto stride = std::stoi(tmpStr);
      // getSubStr(vec, "\"phyId\"", ", ", 8, tmpStr);
      // auto physicalId = tmpStr.c_str();
      // auto physicalId = std::stoi(tmpStr);
      getSubStr(vec, "\"usage\"", ",", 8, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto usage1 = std::stoi(tmpStr);
      // getSubStr(vec, "\"size\"", ",", 7, tmpStr);
      // auto bufSize = std::stoi(tmpStr);
      getSubStr(vec, "\"source\"", ",", 9, tmpStr);
      MY_LOGI_IF(mDebugLevel > 1, "tmpStr:%s", tmpStr.c_str());
      auto streamSource = std::stoi(tmpStr);
      //
      Stream oneOutStream;
      oneOutStream.id = streamId;
      oneOutStream.streamType = StreamType::OUTPUT;
      oneOutStream.width = static_cast<uint32_t>(width);
      oneOutStream.height = static_cast<uint32_t>(height);
      oneOutStream.format = static_cast<NSCam::EImageFormat>(format);
      oneOutStream.usage = static_cast<uint64_t>(usage1);
      oneOutStream.dataSpace = mcam::Dataspace::UNKNOWN;
      oneOutStream.transform = NSCam::eTransform_None;
      // oneOutStream.physicalCameraId = static_cast<uint32_t>(physicalId);
      oneOutStream.physicalCameraId =
          -1;  // TODO(MTK): need implement "physicalCameraId"
      if (streamSource != 0) {
        IMetadata::setEntry<MINT32>(&oneOutStream.settings,
                                    MTK_HALCORE_STREAM_SOURCE, streamSource);
        if (streamSource == MTK_HALCORE_STREAM_SOURCE_ME) {
          meStreamId = streamId;
          MY_LOGD("meStreamId:%d", meStreamId);
        }
      }
      calcuBufPlane(width, height, stride, oneOutStream.format,
                    oneOutStream.bufPlanes);
      oneOutStream.bufferSize = 0;
      for (size_t i = 0; i < oneOutStream.bufPlanes.count; i++) {
        oneOutStream.bufferSize +=
            static_cast<uint32_t>(oneOutStream.bufPlanes.planes[i].sizeInBytes);
      }
      outStream.push_back(oneOutStream);
      MY_LOGI("streamId:%d, width:%d, height:%d, strides:%d, bufSize:%d",
              streamId, width, height, stride, oneOutStream.bufferSize);
      checkStrideAlign(stride, oneOutStream.format);
      // MY_LOGD("%s",toString(oneOutStream));
    }
  };
  // start parse config stream info
  parseInfo(lhsStreamConfigStrings.outputStreams, lhsConfigOutputStreams);
  toLog("lhsConfigOutputStreams", lhsConfigOutputStreams);
  parseInfo(rhsStreamConfigStrings.inputStreams, rhsConfigInputStreams);
  toLog("rhsConfigInputStreams", rhsConfigInputStreams);
  parseInfo(rhsStreamConfigStrings.outputStreams, rhsConfigOutputStreams);
  toLog("rhsConfigOutputStreams", rhsConfigOutputStreams);

  // set input info
  for (auto& stream : rhsConfigInputStreams) {
    stream.streamType = StreamType::INPUT;
  }
  //
  // start parse request info
  // LHS
  for (auto vec : lhsRequestStrings) {
    struct input hal3In;
    struct output hal3Out;
    struct req reqOne;
    int inputBeg = 0;
    int outputBeg = 0;
    for (auto num = 0; num < vec.info.size(); num++) {
      std::string str = (vec.info)[num];
      // MY_LOGI("hal3 request: %s",str.c_str());
      if (str.find("inputBuffer") != std::string::npos) {
        inputBeg = num;
        // MY_LOGI("hal3 inputBeg:%d", inputBeg);
      }
      if (str.find("outputBuffer") != std::string::npos) {
        outputBeg = num;
        // MY_LOGI("hal3 outputBeg:%d", outputBeg);
      }
      if (str.find("id") != std::string::npos && outputBeg == 0) {
        // MY_LOGI("hal3 input id:%s", str.c_str());
        getSubStr_vec(str, "[", ",", "]", &(hal3In.id));
      }
      if (str.find("id") != std::string::npos && outputBeg != 0) {
        // MY_LOGI("hal3 output id:%s", str.c_str());
        getSubStr_vec(str, "[", ",", "]", &(hal3Out.id));
      }
    }
    reqOne.reqInput = hal3In;
    reqOne.reqOutput = hal3Out;
    lhsRequestIds.push_back(reqOne);
  }
  for (auto vec : lhsRequestIds) {
    MY_LOGI("hal3 request:");
    for (auto id : vec.reqInput.id) {
      MY_LOGI("hal3 request input stream id %d ", id);
    }
    for (auto id : vec.reqOutput.id) {
      MY_LOGI("hal3 request output stream id:%d ", id);
    }
  }

  // RHS
  for (auto vec : rhsRequestStrings) {
    struct input ispIn;
    struct output ispOut;
    struct req reqOne;
    int inputBeg = 0;
    int outputBeg = 0;
    for (auto num = 0; num < vec.info.size(); num++) {
      std::string str = (vec.info)[num];
      // MY_LOGI("isp request: %s",str.c_str());
      if (str.find("inputBuffer") != std::string::npos) {
        inputBeg = num;
        // MY_LOGI("isp inputBeg:%d", inputBeg);
      }
      if (str.find("outputBuffer") != std::string::npos) {
        outputBeg = num;
        // MY_LOGI("isp outputBeg:%d", outputBeg);
      }
      if (str.find("id") != std::string::npos && outputBeg == 0) {
        // MY_LOGI("isp input id:%s", str.c_str());
        getSubStr_vec(str, "[", ",", "]", &(ispIn.id));
      }
      if (str.find("id") != std::string::npos && outputBeg != 0) {
        // MY_LOGI("isp output id:%s", str.c_str());
        getSubStr_vec(str, "[", ",", "]", &(ispOut.id));
      }
    }
    reqOne.reqInput = ispIn;
    reqOne.reqOutput = ispOut;
    rhsRequestIds.push_back(reqOne);
  }
  for (auto vec : rhsRequestIds) {
    MY_LOGI("isp request:");
    for (auto id : vec.reqInput.id) {
      MY_LOGI("isp request input stream id %d ", id);
    }
    for (auto id : vec.reqOutput.id) {
      MY_LOGI("isp request output stream id:%d ", id);
    }
  }
  //
  MY_LOGI("lhsRequestIds size:%lu,rhsRequestIds size:%lu", lhsRequestIds.size(),
          rhsRequestIds.size());
  //////////////////////////////////////////
  // hard code for set config / request metadata

  // for config:
  // lhsConfigSetting / rhsConfigSetting
  if (bEnableNativeDevice) {
    // **
    // * MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES
    // *
    // * Configuring time bit mask to desdribe features to be enabled.
    // *
    // mtk_camera_metadata_enum_nativedev_ispvdo_available_features
    MINT64 configFeature = MTK_NATIVEDEV_ISPVDO_AVAILABLE_FEATURES_YUVMCNR;
    IMetadata::setEntry<MINT64>(
        &rhsConfigSetting, MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES, configFeature);
    MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES(%d), %d",
            MTK_NATIVEDEV_ISPVDO_CONFIG_FEATURES, configFeature);

    // **
    // * MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL
    // *
    // * This tag is used to specify controls.
    // *
    std::vector<MINT64> featureControl = {
        // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
        MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_INLINE_WRAP_ENABLE,
        0,  // value of [0]
            // mtk_camera_metadata_enum_nativedev_ispvdo_feature_control
        MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_PQ_DIP_ENABLE,
        0  // value of [2]
    };
#define WIDTH_THREAD_FOR_DE_LEVEL_4 3800
    if (rhsConfigInputStreams[0].width > WIDTH_THREAD_FOR_DE_LEVEL_4) {
      featureControl.push_back(
          MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_DOWNSCALE_LEVEL);
      featureControl.push_back(MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_4);
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_4");
    } else {
      featureControl.push_back(
          MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL_YUVMCNR_DOWNSCALE_LEVEL);
      featureControl.push_back(MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_2);
      MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_YUVMCNR_DOWNSCALE_LEVEL_2");
    }
    trySetVectorToMetadata(&rhsConfigSetting,
                           MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL,
                           featureControl);
    MY_LOGD(
        "MCNR meta: MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL(%d), "
        "featureControl.size(%d)",
        MTK_NATIVEDEV_ISPVDO_FEATURE_CONTROL, featureControl.size());

    // **
    // * MTK_NATIVEDEV_ISPVDO_TUNING_HINT
    // *
    // * External tuning hint. This tag can be NULL, use
    // MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT as default.
    // *
    int64_t tuningNum =
        rhsConfigInputStreams.size();  // equals to input stream size
    std::vector<MINT64> tuningHint;
    for (auto vec : rhsConfigInputStreams) {
      tuningHint.push_back(vec.id);  // stream ID
      tuningHint.push_back(
          // mtk_camera_metadata_enum_nativedev_ispvdo_tuning_hint
          MTK_NATIVEDEV_ISPVDO_TUNING_HINT_DEFAULT);
    }
    trySetVectorToMetadata(&rhsConfigSetting, MTK_NATIVEDEV_ISPVDO_TUNING_HINT,
                           tuningHint);
    MY_LOGD("MCNR meta: MTK_NATIVEDEV_ISPVDO_TUNING_HINT(%d), tuningNum:%d",
            MTK_NATIVEDEV_ISPVDO_TUNING_HINT, tuningNum);

    // **
    // * MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP
    // *
    // * This tag describes the relationship between Stream IDs and
    // feature-based I/O.
    // * Caller need to define the stream usage of each input stream for
    // YUV_MCNR.
    // *
    std::vector<MINT64> streamUsageMap;
    streamUsageMap.push_back(rhsConfigInputStreams[0].id);  // stream ID of f0
    streamUsageMap.push_back(
        // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
        MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F0);
    MY_LOGD("set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F0:%d",
            rhsConfigInputStreams[0].id);
    if (runCase == MCNR_F0_F1) {
      streamUsageMap.push_back(rhsConfigInputStreams[1].id);  // stream ID of f1
      streamUsageMap.push_back(
          // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
          MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1);
      MY_LOGD("set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1:%d",
              rhsConfigInputStreams[1].id);
      trySetVectorToMetadata(&rhsConfigSetting,
                             MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP,
                             streamUsageMap);
    } else if (runCase == MCNR_F0_F1_ME) {
      streamUsageMap.push_back(rhsConfigInputStreams[1].id);  // stream ID of f1
      streamUsageMap.push_back(
          // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
          MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1);
      MY_LOGD("set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_F1:%d",
              rhsConfigInputStreams[1].id);
      streamUsageMap.push_back(rhsConfigInputStreams[2].id);  // stream ID of ME
      streamUsageMap.push_back(
          // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
          MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_ME);
      MY_LOGD("set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_ME:%d",
              rhsConfigInputStreams[2].id);
      streamUsageMap.push_back(TME_STREAM_ID);  // stream ID of tME
      streamUsageMap.push_back(
          // mtk_camera_metadata_enum_nativedev_ispvdo_stream_usage_map
          MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_TME);
      MY_LOGD("set MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_YUVMCNR_TME:%d",
              TME_STREAM_ID);
    }
    trySetVectorToMetadata(&rhsConfigSetting,
                           MTK_NATIVEDEV_ISPVDO_STREAM_USAGE_MAP,
                           streamUsageMap);
  }

  /////////////////////////////////////////////////////////
  // for request:

  // lhsRequestSetting / rhsRequestSetting
  //
  {
    MRect region(MPoint(0, 0), MSize(4000, 3000));
    IMetadata::setEntry<MRect>(&lhsRequestSetting, MTK_SCALER_CROP_APP_REGION,
                               region);
    //
    MFLOAT ratio = 1.0;
    IMetadata::setEntry<MFLOAT>(&lhsRequestSetting, MTK_CONTROL_ZOOM_RATIO,
                                ratio);
  }
  //
  if (deviceType == "ISP_VIDEO") {
    // **
    // * MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI
    // *
    // * NativeDevice supports crop function at output image. If not given, use
    // the full source image size as ROI
    // *
    int64_t ROIPositionX = 0, ROIPositionY = 0;           // ROI's position
    int64_t ROIArrayWidth = 4000, ROIArrayHeight = 3000;  // ROI's size
    std::vector<MINT64> destROI = {rhsConfigInputStreams[0].id,  // stream ID
                                   ROIPositionX, ROIPositionY, ROIArrayWidth,
                                   ROIArrayHeight};
    trySetVectorToMetadata(&rhsRequestSetting,
                           MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI, destROI);
    MY_LOGD(
        "MCNR meta: MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI(%d), stream:%d, "
        "position:[%d,%d], size:[%d,%d]",
        MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ROI, rhsConfigInputStreams[0].id,
        destROI[0], destROI[1], destROI[2], destROI[3], destROI[4]);
    // **
    // * MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION
    // *
    // * NativeDevice supports rotation (0 / 90 / 180 / 270) at the output
    // image(s)
    // *
    std::vector<MINT64> destOrientation = {
        rhsConfigInputStreams[0].id,  // stream ID
        0                             // Orientation value
    };
    trySetVectorToMetadata(&rhsRequestSetting,
                           MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION,
                           destOrientation);
    MY_LOGD(
        "MCNR meta: MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION(%d), "
        "stream:%d, position:%d",
        MTK_NATIVEDEV_ISPVDO_DESTINATION_IMAGE_ORIENTATION, destOrientation[0],
        destOrientation[1]);

  } else if (deviceType == "WPE_PQ") {  // add WPE_PQ request meta
    // **
    // * MTK_POSTPROCDEV_WPEPQ_OUTPUT_SIZE
    // *
    // * Describe the output image size of WPE module. The output image size
    // should smaller or equal to input image size.
    // *
    int64_t outputWidth =
        ::property_get_int32("vendor.debug.hal3ut.wpe.outputw", 1920);
    int64_t outputHeight =
        ::property_get_int32("vendor.debug.hal3ut.wpe.outputh", 1080);
    std::vector<MINT64> outputSize = {outputWidth, outputHeight};
    trySetVectorToMetadata(&rhsRequestSetting, MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE,
                           outputSize);
    MY_LOGD("WPE_PQ meta: MTK_POSTPROCDEV_WPEPQ_OUTPUT_SIZE(%d), [%d,%d]",
            MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, outputSize[0], outputSize[1]);

    // **
    // * MTK_POSTPROCDEV_WPEPQ_FLIP
    // *
    // * Describe the output image need to flip or not of WPE_PQ device.
    // *
    // 0:disable, 1:flip vertical, 2:flip horizontal
    MUINT8 flip = ::property_get_int32("vendor.debug.hal3ut.wpe.flip", 0);
    IMetadata::setEntry<MUINT8>(&rhsRequestSetting, MTK_POSTPROCDEV_WPEPQ_FLIP,
                                flip);
    MY_LOGD("WPE_PQ meta: MTK_POSTPROCDEV_WPEPQ_FLIP(%d), %d",
            MTK_POSTPROCDEV_WPEPQ_FLIP, flip);

    // **
    // * MTK_POSTPROCDEV_WPEPQ_ROTATE
    // *
    // * Describe the output image need to flip or not of WPE_PQ device
    // *
    // value from mtk_camera_metadata_enum_nativedev_wpepq_rotate
    MUINT8 rotate = ::property_get_int32("vendor.debug.hal3ut.wpe.rotate",
                                         MTK_POSTPROCDEV_WPEPQ_ROTATE_NONE);
    IMetadata::setEntry<MUINT8>(&rhsRequestSetting, MTK_POSTPROCDEV_WPEPQ_ROTATE,
                                rotate);
    MY_LOGD("WPE_PQ meta: MTK_POSTPROCDEV_WPEPQ_ROTATE(%d), %d",
            MTK_POSTPROCDEV_WPEPQ_ROTATE, rotate);

    // **
    // * MTK_POSTPROCDEV_WPEPQ_CROP_INFO
    // *
    // * Describe the crop info of WPE_PQ device
    // *
    std::vector<MINT32> crop = {0, 0, 1920, 1080};
    crop[0] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropx", 0);
    crop[1] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropy", 0);
    crop[2] = ::property_get_int32("vendor.debug.hal3ut.wpe.cropw", 1920);
    crop[3] = ::property_get_int32("vendor.debug.hal3ut.wpe.croph", 1080);
    trySetVectorToMetadata(&rhsRequestSetting, MTK_POSTPROCDEV_WPEPQ_CROP_INFO,
                           crop);
    MY_LOGD(
        "WPE_PQ meta: MTK_POSTPROCDEV_WPEPQ_CROP_INFO(%d), position:[%d,%d], "
        "size:[%d,%d]",
        MTK_POSTPROCDEV_WPEPQ_CROP_INFO, crop[0], crop[1], crop[2], crop[3]);
  }
  //
  FUNC_END
  return 0;
}

/////////////////////////////////////////////

MtkHalUt::MtkHalUt()
    : mOpenId(0),
      mTotalRequestNum(30),
      mRunCase(BASIC_YUV_STREAM),
      mbEnableTuningData(false),
      mbEnableNativeDevice(false) {
  MY_LOGI("construct MtkHalUt");
  // mAllocator = IAllocator::getService();
  // mMapper = IMapper::getService();
  mbEnableDump = ::property_get_int32("vendor.debug.mtkhalut.enable.dump", 1);
  mDebugLevel = getDebugLevel();
  mMaxBuffers = 1;
}

MtkHalUt::~MtkHalUt() {
  FUNC_START
  FUNC_END
}

void MtkHalUt::onLastStrongRef(const void* id) {
  FUNC_START
  //
  /*
  auto clearInFlightMap = [](InFlightMap& inflightMap) {
    MY_LOGD("Aroha Test:inflightMap.size(%d)",inflightMap.size());
    for (auto it = inflightMap.begin(); it != inflightMap.end();) {
      MY_LOGD("Aroha Test:111");
      it->second = nullptr;
      MY_LOGD("Aroha Test:112");
      inflightMap.erase(it);
      MY_LOGD("Aroha Test:113");
    }
  };
  auto clearBufferVector = [](std::vector<StreamBuffer>& bufVec) {
    MY_LOGD("Aroha Test:std::vector<StreamBuffer>.size(%d)",bufVec.size());
    for (auto it = bufVec.begin(); it != bufVec.end();) {
      MY_LOGD("Aroha Test:211");
      it->buffer = nullptr;
      MY_LOGD("Aroha Test:212");
      it = bufVec.erase(it);
      MY_LOGD("Aroha Test:213");
    }
  };
  auto clearBufferMap =
      [&clearBufferVector](std::map<int, std::vector<StreamBuffer>>& bufMap) {
      MY_LOGD("Aroha Test:std::map<int,
  std::vector<StreamBuffer>.size(%d)",bufMap.size()); for (auto it =
  bufMap.begin(); it != bufMap.end();) { MY_LOGD("Aroha Test:311");
          clearBufferVector(it->second);
          MY_LOGD("Aroha Test:312");
          bufMap.erase(it);
          MY_LOGD("Aroha Test:313");
        }
      };
  //
  MY_LOGD("Aroha Test:1");
  clearInFlightMap(mLhsInflightMap);
  MY_LOGD("Aroha Test:2");
  clearInFlightMap(mRhsInflightMap);
  //
  MY_LOGD("Aroha Test:3");
  clearBufferMap(mLhsConfigOutputBufMap);
  MY_LOGD("Aroha Test:4");
  clearBufferMap(mRhsConfigOutputBufMap);
  MY_LOGD("Aroha Test:5");
  clearBufferMap(mLhsTotalAvailableBufMap);
  MY_LOGD("Aroha Test:6");
  clearBufferMap(mLhsEnquedBufMap);

  MY_LOGD("Aroha Test:7");
  clearBufferMap(mRhsTotalAvailableBufMap);
  MY_LOGD("Aroha Test:8");
  clearBufferMap(mRhsEnquedBufMap);
  //
  MY_LOGD("Aroha Test:9");
  clearBufferVector(mLhsAllocateOutputBuffers);
  MY_LOGD("Aroha Test:10");
  clearBufferVector(mRhsAllocateOutputBuffers);
  //
  MY_LOGD("Aroha Test:11");
  mLhsReceivedFrameNumber.clear();
  MY_LOGD("Aroha Test:12");
  mRhsReceivedFrameNumber.clear();
*/
  FUNC_END
}

int MtkHalUt::decideLhsTargetNum(std::vector<struct req>& reqs) {
  int num = 0;
  MY_LOGI("reqs size:%lu", reqs.size());
  for (auto r : reqs) {
    auto ids = r.reqOutput.id;
    MY_LOGI("mRunCase:%d, r.reqOutput.id.size:%lu", mRunCase, ids.size());
    if (mRunCase == MCNR_F0_F1_ME && r.reqOutput.id.size() == 3) {
      MY_LOGI("find the MCNR_F0_F1_ME request");
      break;
    }
    if (mRunCase == MCNR_F0_F1 && r.reqOutput.id.size() == 2) {
      MY_LOGI("find the MCNR_F0_F1 request");
      break;
    }
    if (mRunCase == MCNR_F0 && r.reqOutput.id.size() == 1) {
      MY_LOGI("find the  MCNR_F0 request");
      break;
    }
    if (mRunCase == WPE_PQ && r.reqOutput.id.size() == 1) {
      MY_LOGI("find the  WPE_PQ request");
      break;
    }
    if (mRunCase == FD_YUV_STREAM && r.reqOutput.id.size() == 1) {
      MY_LOGI("find basic fdyuv stream request");
      break;
    }
    if (mRunCase == MSNR_RAW_YUV && r.reqOutput.id.size() == 1) {
      MY_LOGI("find the  MSNR_RAW_YUV request");
      break;
    }
    if (r.reqOutput.id.size() == 1) {
      for (auto vec : mLhsConfigOutputStreams) {
        int streamId = vec.id;
        if (mRunCase == BASIC_YUV_STREAM &&
            (vec.format == NSCam::EImageFormat::eImgFmt_YCBCR_420_888 ||
             vec.format == NSCam::EImageFormat::eImgFmt_YUY2 ||
             vec.format == NSCam::EImageFormat::eImgFmt_NV21 ||
             vec.format == NSCam::EImageFormat::eImgFmt_YUV_P010) &&
            streamId == r.reqOutput.id[0]) {
          MY_LOGI("find basic yuv stream request");
          break;
        }
        if (mRunCase == BASIC_RAW_STREAM &&
            vec.format == NSCam::EImageFormat::eImgFmt_RAW16 &&
            streamId == r.reqOutput.id[0]) {
          MY_LOGI("find basic raw stream request");
          break;
        }
      }
    }
    num++;
  }
  return num;
}

int MtkHalUt::decideRhsTargetNum(std::vector<struct req>& reqs) {
  int num = 0;
  MY_LOGI("reqs size:%lu", reqs.size());
  for (auto r : reqs) {
    auto ids = r.reqInput.id;
    MY_LOGI("mRunCase:%d, r.reqInput.id.size:%lu", mRunCase, ids.size());
    if (mRunCase == MCNR_F0_F1_ME && r.reqInput.id.size() == 4) {
      MY_LOGI("find the MCNR_F0_F1_ME request");
      break;
    }
    if (mRunCase == MCNR_F0_F1 && r.reqInput.id.size() == 2) {
      MY_LOGI("find the MCNR_F0_F1 request");
      break;
    }
    if (mRunCase == MCNR_F0 && r.reqInput.id.size() == 1) {
      MY_LOGI("find the  MCNR_F0 request");
      break;
    }
    if (mRunCase == WPE_PQ && r.reqInput.id.size() == 1) {
      MY_LOGI("find the  WPE_PQ request");
      break;
    }
    if (mRunCase == FD_YUV_STREAM && r.reqInput.id.size() == 1) {
      MY_LOGI("find basic fdyuv stream request");
      break;
    }
    if (mRunCase == MSNR_RAW_YUV && r.reqInput.id.size() == 1) {
      MY_LOGI("find the  MSNR_RAW_YUV request");
      break;
    }
    if (r.reqOutput.id.size() == 1) {
      for (auto vec : mLhsConfigOutputStreams) {
        int streamId = vec.id;
        if (mRunCase == BASIC_YUV_STREAM &&
            (vec.format == NSCam::EImageFormat::eImgFmt_YCBCR_420_888 ||
             vec.format == NSCam::EImageFormat::eImgFmt_YUY2 ||
             vec.format == NSCam::EImageFormat::eImgFmt_NV21 ||
             vec.format == NSCam::EImageFormat::eImgFmt_YUV_P010) &&
            streamId == r.reqOutput.id[0]) {
          MY_LOGI("find basic yuv stream request");
          break;
        }
        if (mRunCase == BASIC_RAW_STREAM &&
            vec.format == NSCam::EImageFormat::eImgFmt_RAW16 &&
            streamId == r.reqOutput.id[0]) {
          MY_LOGI("find basic raw stream request");
          break;
        }
      }
    }
    num++;
  }
  return num;
}

bool MtkHalUt::initial() {
  FUNC_START
  //
  std::string log{" ="};
  for (auto const& option : mOptions) {
    log += option;
    log += " ";
  }
  MY_LOGI("options(#%zu)%s", mOptions.size(), log.c_str());
  mPrinter->printFormatLine(
      "## Reporting + (%s)",
      NSCam::Utils::LogTool::get()->getFormattedLogTime().c_str());
  mPrinter->printFormatLine(
      "## Camera Hal Service (camerahalserver pid:%d %zuBIT) ## "
      "options(#%zu)%s",
      ::getpid(), (sizeof(intptr_t) << 3), mOptions.size(), log.c_str());
  //
  // parse options
  parseOptions(log);

  // parse json file
  JsonParser jsonParser;
  jsonParser.setJsonPath(mJSONFilePath);
  jsonParser.parseJson(
      mbEnableNativeDevice, mbEnableTuningData, mRunCase, mOpenId, mDeviceType,
      mLhsConfigSetting, mRhsConfigSetting, mLhsRequestSetting,
      mRhsRequestSetting, mLhsConfigOutputStreams, mRhsConfigInputStreams,
      mRhsConfigOutputStreams, mLhsReqs, mRhsReqs, mMeStreamId);
  //
  // decide target num
  mLhsTargetNum = decideLhsTargetNum(mLhsReqs);
  mRhsTargetNum = decideRhsTargetNum(mRhsReqs);
  MY_LOGI("mLhsTargetNum:%d, mRhsTargetNum:%d", mLhsTargetNum, mRhsTargetNum);
  //
  mOpenId = 0;
  int32_t rhsDeviceId = 0;
  if (mbEnableNativeDevice) {
    if (mDeviceType == "ISP_VIDEO") {
      mRhsRequestTemplate = RequestTemplate::PREVIEW;
      rhsDeviceId = 0;
    } else if (mDeviceType == "ISP_CAPTURE") {
      mRhsRequestTemplate = RequestTemplate::STILL_CAPTURE;
      rhsDeviceId = 2;
    } else if (mDeviceType == "WPE_PQ") {
      mRhsRequestTemplate = RequestTemplate::PREVIEW;
      rhsDeviceId = 1;
    } else {
      mRhsRequestTemplate = RequestTemplate::PREVIEW;
      rhsDeviceId = 3;  // FD
    }
  } else {
    mLhsRequestTemplate = RequestTemplate::PREVIEW;
  }

  mOperationMode = mcam::StreamConfigurationMode::NORMAL_MODE;

  //
  if (mOpenId >= mCameraIdList.size()) {
    MY_LOGE("mOpenId(%d) >= mCameraIdList.size(%zu)", mOpenId,
            mCameraIdList.size());
    return false;
  }
  mInstanceId = mCameraIdList[mOpenId];
  MY_LOGD("intance id:%d, rhsDeviceId:%d", mInstanceId, rhsDeviceId);

  // LHS part
  {
    // (1) get provider
    IProviderCreationParams mtkParams = {
        .providerName = "MtkHalUt_LHS",
    };
    // mLhsProvider = getMtkcamCameraProviderInstance(&mtkParams);
    mLhsProvider = getMtkcamProviderInstance(&mtkParams);
    if (mLhsProvider == nullptr) {
      MY_LOGE("mLhsProvider is null!");
      return false;
    }
    // (2) get device from provider
    if (mLhsProvider->getDeviceInterface(mInstanceId, mLhsDevice) != 0) {
      MY_LOGE("mLhsProvider getDeviceInterface fail!");
      return false;
    }

    // (3-1) create session from devices
    // (3-2) set callback
    auto pListener = CallbackListener::create(this, true);
    mLhsListener = std::shared_ptr<CallbackListener>(pListener);
    mLhsSession = mLhsDevice->open(mLhsListener);
    if (mLhsSession == nullptr) {
      MY_LOGE("mLhsSession is null!");
      return false;
    }

    // (4) get static metadata
    if (mLhsDevice->getCameraCharacteristics(mCameraCharacteristics) != 0) {
      MY_LOGE("getCameraCharacteristics fail!");
      return false;
    }
  }
  // RHS part
  if (mbEnableNativeDevice) {
    // (1) get provider
    IProviderCreationParams mtkParams = {
        .providerName = "MtkHalUt_RHS",
    };
    // mRhsProvider = getMtkcamPostProcProviderInstance(&mtkParams);
    // mRhsProvider = getPostProcProviderInstance(&mtkParams);
    if (mRhsProvider == nullptr) {
      MY_LOGE("mRhsProvider is null!");
      return false;
    }
    // (2) get device from provider
    if (mRhsProvider->getDeviceInterface(rhsDeviceId, mRhsDevice) != 0) {
      MY_LOGE("mRhsProvider getDeviceInterface fail!");
      return false;
    }

    // (3-1) create session from devices
    // (3-2) set callback
    auto pListener = CallbackListener::create(this, false);
    mRhsListener = std::shared_ptr<CallbackListener>(pListener);
    mRhsSession = mRhsDevice->open(mRhsListener);
    if (mRhsSession == nullptr) {
      MY_LOGE("mRhsSession is null!");
      return false;
    }

    // (4) get static metadata
    // if(mRhsDevice->getCameraCharacteristics(mCameraCharacteristics)!=0) {
    //  MY_LOGE("getCameraCharacteristics fail!");
    //  return false;
    //}
  }

  FUNC_END
  return true;
}

int MtkHalUt::config() {
  FUNC_START
  // LHS part
  mcam::StreamConfiguration streamConfiguration;
  mcam::HalStreamConfiguration halStreamConfiguration;

  int ret = 0;
  IMetadata tempSetting;
  ret = mLhsSession->constructDefaultRequestSettings(mLhsRequestTemplate,
                                                     tempSetting);
  MY_LOGA_IF(ret != 0, "constructDefaultRequestSettings fail");
  //
  mLhsConfigSetting = tempSetting + mLhsConfigSetting;
  mLhsRequestSetting = mLhsConfigSetting + mLhsRequestSetting;
  //
  streamConfiguration.streams = mLhsConfigOutputStreams;
  streamConfiguration.sessionParams = mLhsConfigSetting;
  streamConfiguration.streamConfigCounter = 0;  // TODO(MTK): add config count.
  streamConfiguration.operationMode = mOperationMode;
  //
  toLog("LHS streamConfiguration", streamConfiguration.streams);
  //
  ret = mLhsSession->configureStreams(streamConfiguration,
                                      halStreamConfiguration);
  MY_LOGA_IF(ret != 0, "configureStreams fail");

  // check halStreamConfiguration
  for (auto const& stream : halStreamConfiguration.streams) {
    auto id = stream.id;
    auto fmt = stream.overrideFormat;
    auto maxBuffers = stream.maxBuffers;
    MY_LOGD("halStreamConfiguration: id(%d) fmt(0x%X) maxBuffers(%d)", id, fmt,
            maxBuffers);
    //
    if (mMaxBuffers < maxBuffers)
      mMaxBuffers = maxBuffers;
  }
  //
  mMaxBuffers = 12;
  //
  MY_LOGD("mMaxBuffers:%d", mMaxBuffers);
  // RHS part
  if (mbEnableNativeDevice) {
    mcam::StreamConfiguration streamConfiguration;
    mcam::HalStreamConfiguration halStreamConfiguration;

    int ret = 0;
    IMetadata tempSetting;
    ret = mRhsSession->constructDefaultRequestSettings(mRhsRequestTemplate,
                                                       tempSetting);
    MY_LOGA_IF(ret != 0, "constructDefaultRequestSettings fail");
    //
    mRhsConfigSetting = tempSetting + mRhsConfigSetting;
    mRhsRequestSetting = mRhsConfigSetting + mRhsRequestSetting;
    //
    std::vector<Stream> totalConfigStream = mRhsConfigInputStreams;
    totalConfigStream.insert(totalConfigStream.end(),
                             mRhsConfigOutputStreams.begin(),
                             mRhsConfigOutputStreams.end());
    //
    streamConfiguration.streams = totalConfigStream;
    streamConfiguration.sessionParams = mRhsConfigSetting;
    streamConfiguration.streamConfigCounter =
        0;  // TODO(MTK): add config count.
    streamConfiguration.operationMode = mOperationMode;
    //
    toLog("RHS streamConfiguration", streamConfiguration.streams);
    //
    ret = mRhsSession->configureStreams(streamConfiguration,
                                        halStreamConfiguration);
    MY_LOGA_IF(ret != 0, "configureStreams fail");
  }
  FUNC_END
  return 0;
}

static IMetadata g_HalResult;

class CbListener : public IMtkcamDeviceCallback {
 public:
  virtual auto processCaptureResult(const std::vector<CaptureResult>& results)
      -> void;

  virtual auto notify(const std::vector<NotifyMsg>& msgs) -> void;
};

auto CbListener::processCaptureResult(const std::vector<CaptureResult>& results)
    -> void {
  auto dumpStreamBuffer = [](const StreamBuffer& streamBuf,
                             std::string prefix_msg, uint32_t frameNumber) {
    CAM_ULOGMD("[dumpStreamBuffer] +");
    //
    auto streamId = streamBuf.streamId;
    auto pImgBufHeap = streamBuf.buffer;
    if (!pImgBufHeap) {
      CAM_ULOGMD("[dumpStreamBuffer] pImgBufHeap is null!");
      CAM_ULOGMD("[dumpStreamBuffer] -");
      return;
    }
    auto imgBitsPerPixel = pImgBufHeap->getImgBitsPerPixel();
    auto imgSize = pImgBufHeap->getImgSize();
    auto imgFormat = pImgBufHeap->getImgFormat();
    auto imgStride = pImgBufHeap->getBufStridesInBytes(0);
    std::string extName;
    //
    if (imgFormat == NSCam::eImgFmt_JPEG) {
      extName = "jpg";
    } else if (imgFormat == NSCam::eImgFmt_RAW16 ||
               (imgFormat >= 0x2200 && imgFormat <= 0x2300)) {
      extName = "raw";
    } else {
      extName = "yuv";
    }
    //
    char szFileName[1024];
    snprintf(szFileName, sizeof(szFileName), "%s%s-PW_%d_%dx%d_%d_%d.%s",
             DUMP_FOLDER, prefix_msg.c_str(), frameNumber, imgSize.w, imgSize.h,
             imgStride, streamId, extName.c_str());
    //
    auto imgBuf = pImgBufHeap->createImageBuffer();
    //
    MY_LOGD("dump image frameNum(%d) streamId(%d) to %s", frameNumber, streamId,
            szFileName);
    imgBuf->lockBuf(prefix_msg.c_str(), mcam::eBUFFER_USAGE_SW_READ_OFTEN);
    imgBuf->saveToFile(szFileName);
    imgBuf->unlockBuf(prefix_msg.c_str());
    //
    CAM_ULOGMD("[dumpStreamBuffer] -");
  };
  //
  CAM_ULOGMD("[processCaptureResult] +");
  CAM_ULOGMD("[processCaptureResult] results size:%d", results.size());
  for (size_t i = 0; i < results.size(); i++) {
    auto frameNumber = results[i].frameNumber;
    auto outBufSize = results[i].outputBuffers.size();
    auto inBufSize = results[i].inputBuffers.size();
    auto haveLast = results[i].bLastPartialResult;
    auto appMetaCnt = results[i].result.count();
    auto halMetaCnt = results[i].halResult.count();
    g_HalResult = g_HalResult + results[i].halResult;
    CAM_ULOGMD(
        "[processCaptureResult] i:%d frameNo:%d outSize:%d inSize:%d last:%d "
        "app:%d hal:%d g_HalResult:%d",
        i, frameNumber, outBufSize, inBufSize, haveLast, appMetaCnt, halMetaCnt,
        g_HalResult.count());
    for (auto& streamBuf : results[i].outputBuffers) {
      dumpStreamBuffer(streamBuf, "Debug", frameNumber);
    }
  }
  CAM_ULOGMD("[processCaptureResult] -");
}

auto CbListener::notify(const std::vector<NotifyMsg>& msgs) -> void {
  CAM_ULOGMD("[notify] +");
  CAM_ULOGMD("[notify] msgs.size(%d)", msgs.size());
  for (size_t i = 0; i < msgs.size(); i++) {
    CAM_ULOGMD("[notify] msgs[%d].type(%d)", i, msgs[i].type);
  }
  CAM_ULOGMD("[notify] -");
}

void testJson() {
  // Json::Value testCaseSetting;
  string jsonPath = "/data/vendor/camera_dump/new_new_new_ut_1_3_b.json";

  MtkHalData mtkHalData;
  JsonData jsonData;

  JSonUtil jsonUtil;

  JSonUtil::ParseResult result =
      jsonUtil.ParseJsonFile(jsonPath.c_str(), jsonData);
  if (result != JSonUtil::SUCCESSFUL) {
    MY_LOGE("ParseJsonFile fail: ret=%d", (int)result);
    return;
  }

  MY_LOGD("dumpJsonData +");

  JSonUtil::dumpJsonData(jsonData);

  MY_LOGD("dumpJsonData -");

  IMetadata tmp;
  if (!mtkHalData.transformFromJsonData(jsonData, tmp)) {
    MY_LOGE("transformFromJsonData fail");
    return;
  }

  MY_LOGD("dumpData +");
  mtkHalData.dumpData();
  MY_LOGD("dumpData -");
}

void MtkHalTest::start(const std::shared_ptr<IPrinter> printer,
                       const std::vector<std::string>& options,
                       const std::vector<int32_t>& cameraIdList) {
  //
  std::shared_ptr<IMtkcamProvider> pLhsProvider;
  std::shared_ptr<IMtkcamDevice> pLhsDevice;
  std::shared_ptr<IMtkcamDeviceSession> pLhsSession;
  std::shared_ptr<CallbackListener> pLhsListener;
  //
  std::shared_ptr<IMtkcamProvider> pRhsProvider;
  std::shared_ptr<IMtkcamDevice> pRhsDevice;
  std::shared_ptr<IMtkcamDeviceSession> pRhsSession;
  std::shared_ptr<CallbackListener> pRhsListener;
  //
  ////////////////////////////////////////////////////////////////////
  auto openMtkHal = [&, this](bool isLhs, string const& providerName,

                              int deviceID,
                              std::shared_ptr<IMtkcamProvider>& pProvider,
                              std::shared_ptr<IMtkcamDevice>& pDevice,
                              std::shared_ptr<IMtkcamDeviceSession>& pSession,
                              std::shared_ptr<CallbackListener>& pListener) {
    // (1) get provider instance
    IProviderCreationParams mtkParams = {
        .providerName = providerName,
    };
    if (isLhs)
      pProvider = getMtkcamProviderInstance(&mtkParams);
    else
      pProvider = getPostProcProviderInstance(&mtkParams);
    if (pProvider == nullptr) {
      CAM_LOGA("[%s] pProvider is null!", providerName.c_str());
      return false;
    }
    // (2) get device from provider
    if (pProvider->getDeviceInterface(deviceID, pDevice) != 0) {
      CAM_LOGA("[%s] pProvider getDeviceInterface fail!", providerName.c_str());
      return false;
    }

    // (3-1) create session from devices
    // (3-2) set callback
    auto tmpListener = CallbackListener::create(this, isLhs);
    pListener = std::shared_ptr<CallbackListener>(tmpListener);
    pSession = pDevice->open(pListener);
    if (pSession == nullptr) {
      CAM_LOGA("[%s] pSession is null!", providerName.c_str());
      return false;
    }
    //
    return true;
  };
  ////////////////////////////////////////////////////////////////////
  //
  // 01.parse debug cmd option
  mCmdOptionParser.setCmdOption(options);
  string jsonPath = mCmdOptionParser.getCmdOption("-p");
  MY_LOGD("getCmdOption(-p):%s", jsonPath.c_str());
  //
  printer->printFormatLine("Loading JSon Setting...");
  // 02.load and parse JSon file
  if (!LoadJsonFile(jsonPath)) {
    MY_LOGA("LoadJsonFile(%s) Fail!", jsonPath.c_str());
    return;
  }
  // for debug: dump JSon content
  JSonUtil::dumpJsonData(mJsonData);
  // 03.decide the flow by JSon setting
  FlowSettingPolicy();
  // 04.open Mtk Hal Interface
  // 04-1.open LHS part
  printer->printFormatLine("open LHS device...");
  {
    string provideName = "MtkHalTest_LHS";
    if (!openMtkHal(true, provideName, mCameraDeviceId, pLhsProvider,
                    pLhsDevice, pLhsSession, pLhsListener)) {
      MY_LOGA("openMtkHal(%s) Fail!", provideName.c_str());
      return;
    }
  }
  // 04-2.open RHS part
  printer->printFormatLine("open RHS device...");
  if (mEnableRhsFlow) {
    string provideName = "MtkHalTest_RHS";
    if (!openMtkHal(false, provideName, mRhsDeviceId, pRhsProvider,
                    pRhsDevice, pRhsSession, pRhsListener)) {
      MY_LOGA("openMtkHal(%s) Fail!", provideName.c_str());
      return;
    }
  }

  // 05.construct default app metadata setting
  IMetadata templateSetting;
  pLhsSession->constructDefaultRequestSettings(mTemplateType, templateSetting);
  //
  if (!mMtkHalData.transformFromJsonData(mJsonData, templateSetting)) {
    MY_LOGA("transformFromJsonData Fail!");
    return;
  }
  // debug dump mMtkHalData;
  mMtkHalData.dumpData();
  //
  // 06.allocate output image buffer heap
  printer->printFormatLine("allocate buffer...");
  if (!mMtkHalData.allocateStreamBuffer()) {
    MY_LOGA("allocateStreamBuffer Fail!");
    return;
  }

  // 07.config
  // 07-1.config LHS
  StreamConfiguration lhsStreamConfiguration;
  printer->printFormatLine("config LHS device...");
  if (!mMtkHalData.getLhsConfigData(lhsStreamConfiguration)) {
    MY_LOGA("getLhsConfigData fail!");
    return;
  }
  //
  toLog("lhsConfigOutputStreams", lhsStreamConfiguration.streams);
  //
  HalStreamConfiguration lhsHalStreamConfiguration;
  if (pLhsSession->configureStreams(lhsStreamConfiguration,
                                    lhsHalStreamConfiguration) != 0) {
    MY_LOGA("configureStreams LHS fail!");
    return;
  }
  // 07-2.config RHS
  printer->printFormatLine("config RHS device...");
  if (mEnableRhsFlow) {
    StreamConfiguration rhsStreamConfiguration;
    if (!mMtkHalData.getRhsConfigData(rhsStreamConfiguration)) {
      MY_LOGA("getRhsConfigData fail!");
      return;
    }
    //
    toLog("rhsConfigOutputStreams", rhsStreamConfiguration.streams);
    //
    HalStreamConfiguration rhsHalStreamConfiguration;
    if (pRhsSession->configureStreams(rhsStreamConfiguration,
                                      rhsHalStreamConfiguration) != 0) {
      MY_LOGA("configureStreams RHS fail!");
      return;
    }
  }

  // 08.queue LHS & RHS request
  uint32_t numRequestProcessed = 0;
  for (int frameNo = 0; frameNo < mRequestNum; frameNo++) {
    CaptureRequest capReq;
    ////////////////////////////////////////////////////
    // LHS request
    if (!mMtkHalData.getLhsRequestData(frameNo, capReq)) {
      MY_LOGA("getLhsRequestData fail!");
      return;
    }
    std::vector<CaptureRequest> requests;
    requests.push_back(capReq);
    //
    toLog("Lhs mtkRequests", requests);
    //
    {
      MY_LOGD("mLock +");
      std::unique_lock<std::mutex> _Lock(mLock);
      MY_LOGD("mLock -");
      //
      printer->printFormatLine("queue LHS requestNo:%d (%d/%d)...", frameNo,
                               frameNo + 1, mRequestNum);
      if (pLhsSession->processCaptureRequest(requests, numRequestProcessed) !=
          0) {
        MY_LOGA("processCaptureRequest LHS fail!");
        return;
      }
      MY_LOGD("queue LHS request:%d", frameNo);
      //
      InFlightRequest inFlightRequest;
      inFlightRequest.frameNumber = frameNo;
      inFlightRequest.outputBufferNum = capReq.outputBuffers.size();
      mLhsInFlightRequestTable.emplace(frameNo, inFlightRequest);
      //
      mbWaitLhsDone = true;
      MY_LOGD("mCondition.wait +");
      mCondition.wait(_Lock);
      MY_LOGD("mCondition.wait -");
    }

    ////////////////////////////////////////////////////
    // RHS request
    if (mEnableRhsFlow) {
      CaptureRequest capReq;
      if (!mMtkHalData.getRhsRequestData(frameNo, capReq)) {
        MY_LOGA("getRhsRequestData fail!");
        return;
      }
      {
        MY_LOGD("mLock +");
        std::unique_lock<std::mutex> _Lock(mLock);
        MY_LOGD("mLock -");
        // get LHS Hal Result Metadata
        if (mLhsInFlightRequestTable.find(frameNo) !=
            mLhsInFlightRequestTable.end()) {
          auto inFlightReq = mLhsInFlightRequestTable.at(frameNo);
          if (inFlightReq.haveLastResultMetadata) {
            capReq.halSettings = inFlightReq.fullHalResult;
          } else {
            MY_LOGA("frameNo:%d didn't have haveLastResultMetadata!", frameNo);
            return;
          }
        } else {
          MY_LOGA("frameNo:%d not in mLhsInFlightRequestTable!", frameNo);
          return;
        }
        //
        std::vector<CaptureRequest> requests;
        requests.push_back(capReq);
        //
        toLog("Rhs mtkRequests", requests);
        //
        printer->printFormatLine("queue RHS requestNo:%d (%d/%d)...", frameNo,
                                 frameNo + 1, mRequestNum);
        if (pRhsSession->processCaptureRequest(requests, numRequestProcessed) !=
            0) {
          MY_LOGA("processCaptureRequest RHS fail!");
          return;
        }
        MY_LOGD("queue RHS request:%d", frameNo);
        //
        InFlightRequest inFlightRequest;
        inFlightRequest.frameNumber = frameNo;
        inFlightRequest.outputBufferNum = capReq.outputBuffers.size();
        mRhsInFlightRequestTable.emplace(frameNo, inFlightRequest);
        //
        mbWaitRhsDone = true;
        MY_LOGD("mCondition.wait +");
        mCondition.wait(_Lock);
        MY_LOGD("mCondition.wait -");
      }
      MY_LOGD("queue RHS request:%d", frameNo);
    }
  }

  // 09.close device to exit
  MY_LOGD("release flow +");
  pLhsSession->close();
  pLhsProvider = nullptr;
  pLhsDevice = nullptr;
  pLhsSession = nullptr;
  pLhsListener = nullptr;
  //
  if (mEnableRhsFlow) {
    pRhsSession->close();
    pRhsProvider = nullptr;
    pRhsDevice = nullptr;
    pRhsSession = nullptr;
    pRhsListener = nullptr;
  }
  MY_LOGD("release flow -");
}

bool MtkHalTest::LoadJsonFile(string const& sPath) {
  JSonUtil util;
  JSonUtil::ParseResult res = util.ParseJsonFile(sPath.c_str(), mJsonData);
  if (res != JSonUtil::SUCCESSFUL) {
    MY_LOGE("ParseJsonFile(%s) fail! ret=%d", sPath.c_str(), res);
    return false;
  }
  return true;
}

void MtkHalTest::FlowSettingPolicy() {
  mCameraDeviceId = mJsonData.cameraDeviceId;
  mRhsDeviceId = mJsonData.rhsDeviceId;
  mTestCaseType = mJsonData.caseType;
  mRequestNum = std::stoi(mCmdOptionParser.getCmdOption("-count"));
  //
  if (mTestCaseType == "MCNR") {
    mTemplateType = RequestTemplate::PREVIEW;
  }
  //
  mbEnableDump = true;
  if ( mJsonData.Rhs.requestSettings.size() == 0 ) {
    mEnableRhsFlow = false;
    MY_LOGD("mJsonData.Rhs.requestSettings.size() is empty => NO NEED RHS");
  } else {
    mEnableRhsFlow = true;
  }
  mTemplateType = RequestTemplate::PREVIEW;
  //
  MY_LOGD("mRequestNum:%d", mRequestNum);
  MY_LOGD("mCameraDeviceId:%d", mCameraDeviceId);
  MY_LOGD("mTestCaseType:%s", mTestCaseType.c_str());
  MY_LOGD("mEnableRhsFlow:%d", mEnableRhsFlow);
  MY_LOGD("mTemplateType:%d", (int)mTemplateType);
}

void MtkHalTest::LhsNotify(const std::vector<mcam::NotifyMsg>& messages) {
  FUNC_START
  std::unique_lock<std::mutex> l(mLock);
  MY_LOGI("LHS get notify");
  for (size_t i = 0; i < messages.size(); i++) {
    switch (messages[i].type) {
      case MsgType::ERROR:
        MY_LOGE("ERROR:(%d)! frame number: %u, stream id: %d",
                (int)messages[i].msg.error.errorCode,
                messages[i].msg.error.frameNumber,
                messages[i].msg.error.errorStreamId);
        break;
      case MsgType::SHUTTER:
        MY_LOGI("get the shutter: frameNumber:%u timestamp:%llu",
                messages[i].msg.shutter.frameNumber,
                messages[i].msg.shutter.timestamp);
        break;
      default:
        MY_LOGE("Unsupported notify message %d", messages[i].type);
        break;
    }
  }
  FUNC_END
}

bool MtkHalTest::processResult(
    const mcam::CaptureResult& oneResult,
    std::map<uint32_t, InFlightRequest>& inFlightTable,
    std::string& msg) {
  FUNC_START
  MY_LOGD(
      "inputBuffers size:%d, outputBuffers size:%d, "
      "bLastPartialResult:%d, AppResult.size(%d), HalResult.size(%d)",
      oneResult.inputBuffers.size(), oneResult.outputBuffers.size(),
      oneResult.bLastPartialResult, oneResult.result.count(),
      oneResult.halResult.count());
  //
  auto frameNumber = oneResult.frameNumber;
  if ((oneResult.result.isEmpty()) && (oneResult.halResult.isEmpty()) &&
      (oneResult.outputBuffers.size() == 0) &&
      (oneResult.inputBuffers.size() == 0) &&
      (oneResult.bLastPartialResult == false)) {
    MY_LOGE("No result data provided by HAL for frame %d", frameNumber);
    return false;
  }
  //
  auto it = inFlightTable.find(frameNumber);
  if (it == inFlightTable.end()) {
    MY_LOGE("received frameNumber:%u not in inFlightTable", frameNumber);
    return false;
  }
  auto& inFlightRequest = it->second;
  //
  if (oneResult.bLastPartialResult) {
    if (inFlightRequest.haveLastResultMetadata) {
      MY_LOGE("received multi bLastPartialResult!  frameNumber: %u",
              frameNumber);
      return false;
    } else {
      inFlightRequest.haveLastResultMetadata = true;
      MY_LOGD("frameNumber(%d) received bLastPartialResult", frameNumber);
    }
  }
  //
  // collect all metadata and output stream buffers
  if (!oneResult.result.isEmpty()) {
    inFlightRequest.fullAppResult += oneResult.result;
  }
  if (!oneResult.halResult.isEmpty()) {
    inFlightRequest.fullHalResult += oneResult.halResult;
  }
  if (oneResult.outputBuffers.size() != 0) {
    inFlightRequest.resultOutputBuffers.insert(
        inFlightRequest.resultOutputBuffers.end(),
        oneResult.outputBuffers.begin(), oneResult.outputBuffers.end());
    // dump buffer
    if (mbEnableDump) {
      for (auto& streamBuf : oneResult.outputBuffers) {
        dumpStreamBuffer(streamBuf, msg.c_str(), frameNumber);
      }
    }
  }
  //
  if (inFlightRequest.resultOutputBuffers.size() ==
      inFlightRequest.outputBufferNum) {
    inFlightRequest.receivedAllOutputBuffer = true;
    MY_LOGD("frameNumber(%d) received all outputBuffers", frameNumber);
  }
  // dump processed stream buffer
  bool notifyDone = false;
  if (inFlightRequest.receivedAllOutputBuffer &&
      inFlightRequest.haveLastResultMetadata) {
    MY_LOGD("frameNumber(%d) received done!", frameNumber);
    notifyDone = true;
    //
    MY_LOGD(
        "resultOutputBuffers size:%d, "
        "fullAppResult.size(%d), fullHalResult.size(%d)",
        inFlightRequest.resultOutputBuffers.size(),
        inFlightRequest.fullAppResult.count(),
        inFlightRequest.fullHalResult.count());
  }
  //
  FUNC_END
  return notifyDone;
}

void MtkHalTest::LhsProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  FUNC_START
  std::unique_lock<std::mutex> l(mLock);
  MY_LOGD("results size:%d", results.size());
  for (size_t i = 0; i < results.size(); i++) {
    bool notifyDone = false;
    auto frameNumber = results[i].frameNumber;
    std::string msg = "LHS";
    notifyDone = processResult(results[i], mLhsInFlightRequestTable, msg);
    //
    if (notifyDone) {
      MY_LOGI("frameNumber:%d processed done!", frameNumber);
      if (mbWaitLhsDone) {
        mCondition.notify_all();
        mbWaitLhsDone = false;
      }
      // auto it = mInFlightRequestTable.find(frameNumber);
      // if (it!=mInFlightRequestTable.end())
      //  mInFlightRequestTable.erase(it);
      break;
    }
  }
  FUNC_END
}

void MtkHalTest::RhsNotify(const std::vector<mcam::NotifyMsg>& messages) {
  FUNC_START
  std::unique_lock<std::mutex> l(mLock);
  MY_LOGI("RHS get notify");
  for (size_t i = 0; i < messages.size(); i++) {
    switch (messages[i].type) {
      case MsgType::ERROR:
        MY_LOGE("ERROR:(%d)! frame number: %u, stream id: %d",
                (int)messages[i].msg.error.errorCode,
                messages[i].msg.error.frameNumber,
                messages[i].msg.error.errorStreamId);
        break;
      case MsgType::SHUTTER: {
        MY_LOGI("get the shutter: frameNumber:%u timestamp:%llu",
                messages[i].msg.shutter.frameNumber,
                messages[i].msg.shutter.timestamp);
      } break;
      default:
        MY_LOGE("Unsupported notify message %d", messages[i].type);
        break;
    }
  }
  FUNC_END
}

void MtkHalTest::RhsProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  FUNC_START
  std::unique_lock<std::mutex> l(mLock);
  MY_LOGD("results size:%d", results.size());
  for (size_t i = 0; i < results.size(); i++) {
    //
    bool notifyDone = false;
    auto frameNumber = results[i].frameNumber;
    std::string msg = "RHS";
    notifyDone = processResult(results[i], mRhsInFlightRequestTable, msg);
    //
    if (notifyDone) {
      MY_LOGI("frameNumber:%d processed done!", frameNumber);
      if (mbWaitRhsDone) {
        mCondition.notify_all();
        mbWaitRhsDone = false;
      }
      // auto it = mInFlightRequestTable.find(frameNumber);
      // if (it!=mInFlightRequestTable.end())
      //  mInFlightRequestTable.erase(it);
      break;
    }
  }
  FUNC_END
}
void MtkHalTest::dumpStreamBuffer(const StreamBuffer& streamBuf,
                                  std::string prefix_msg,
                                  uint32_t frameNumber) {
  FUNC_START
  auto streamId = streamBuf.streamId;
  auto pImgBufHeap = streamBuf.buffer;
  if (!pImgBufHeap) {
    MY_LOGE("pImgBufHeap is null! streamId:0x%X", streamBuf.streamId);
    FUNC_END
    return;
  }
  auto imgBitsPerPixel = pImgBufHeap->getImgBitsPerPixel();
  auto imgSize = pImgBufHeap->getImgSize();
  auto imgFormat = pImgBufHeap->getImgFormat();
  auto imgStride = pImgBufHeap->getBufStridesInBytes(0);
  std::string extName;
  //
  if (imgFormat == NSCam::eImgFmt_JPEG) {
    extName = "jpg";
  } else if (imgFormat == NSCam::eImgFmt_RAW16 ||
             (imgFormat >= 0x2200 && imgFormat <= 0x2300)) {
    extName = "raw";
  } else {
    extName = "yuv";
  }
  //
  char szFileName[1024];
  snprintf(szFileName, sizeof(szFileName), "%s%s-PW_%d_%dx%d_%d_%d.%s",
           DUMP_FOLDER, prefix_msg.c_str(), frameNumber, imgSize.w, imgSize.h,
           imgStride, streamId, extName.c_str());
  //
  auto imgBuf = pImgBufHeap->createImageBuffer();
  //
  MY_LOGD("dump image frameNum(%d) streamId(%d) to %s", frameNumber, streamId,
          szFileName);
  imgBuf->lockBuf(prefix_msg.c_str(), mcam::eBUFFER_USAGE_SW_READ_OFTEN);
  imgBuf->saveToFile(szFileName);
  imgBuf->unlockBuf(prefix_msg.c_str());
  //
  FUNC_END
}

/******************************************************************************
 *
 ******************************************************************************/
MtkHalTest::CallbackListener::CallbackListener(MtkHalTest* pParent,
                                               bool bIsLhs) {
  mParent = pParent;
  mbIsLhs = bIsLhs;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkHalTest::CallbackListener::create(MtkHalTest* pParent, bool bIsLhs)
    -> CallbackListener* {
  auto pInstance = new CallbackListener(pParent, bIsLhs);
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
void MtkHalTest::CallbackListener::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  if (mbIsLhs)
    mParent->LhsProcessCompleted(results);
  else
    mParent->RhsProcessCompleted(results);
}

/******************************************************************************
 *
 ******************************************************************************/
void MtkHalTest::CallbackListener::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  if (mbIsLhs)
    mParent->LhsNotify(msgs);
  else
    mParent->RhsNotify(msgs);
}

/******************************************************************************
 *
 ******************************************************************************/

void MtkHalUt::start(const std::shared_ptr<IPrinter> printer,
                     const std::vector<std::string>& options,
                     const std::vector<int32_t>& cameraIdList) {
  FUNC_START
  mPrinter = printer;
  mOptions = options;
  mCameraIdList = cameraIdList;
  // initial
  if (!initial()) {
    MY_LOGE("initial fail!");
    return;
  }

  // config
  config();

  // allocate buffer
  MY_LOGI("mMaxBuffers:%d", mMaxBuffers);
  allocateStreamBuffers();
  MY_LOGI("allocat buffers done");

  // queue request
  queueRequest();
  // flush
  // mExtHandler->flush();
  // close
  do {
    std::unique_lock<std::mutex> _lock(mRhsLock);
    mRhsCondition.wait(_lock);
    MY_LOGI("mLhsInflightMap size:%zu", mLhsInflightMap.size());
  } while (mLhsInflightMap.size() != 0);
  FUNC_END
}

int MtkHalUt::getOpenId() {
  return mOpenId;
}

int MtkHalUt::parseOptions(std::string options) {
  FUNC_START
  auto start = options.find("-count");
  auto end = options.find(" ");
  if (start != std::string::npos && end != std::string::npos && start != end) {
    auto str = options.substr(start + 7, end - start - 7);
    mTotalRequestNum = std::stoi(str);
  }
  auto pos = options.find("-t");
  if (pos != std::string::npos) {
    auto subStr = options.substr(pos + 3, 3);
    if (subStr == "0_0") {
      mRunCase = BASIC_YUV_STREAM;
    }
    if (subStr == "0_1") {
      mRunCase = BASIC_RAW_STREAM;
    }
    if (subStr == "0_2") {
      mRunCase = BASIC_YUV_VARIO_ROI;
    }
    if (subStr == "1_0") {
      mRunCase = MCNR_F0;
    }
    if (subStr == "1_2") {
      mRunCase = MCNR_F0_F1;
    }
    if (subStr == "1_3") {
      mRunCase = MCNR_F0_F1_ME;
    }
    if (subStr == "2_1") {
      mRunCase = WPE_PQ;
    }
    if (subStr == "3_1") {
      mRunCase = FD_YUV_STREAM;
    }
  }
  //
  if (mRunCase == MCNR_F0 || mRunCase == MCNR_F0_F1 ||
      mRunCase == MCNR_F0_F1_ME || mRunCase == MCNR_F0_F1_ME_WPE ||
      mRunCase == WPE_PQ) {
    mbEnableTuningData = 1;
    mbEnableNativeDevice = 1;
  } else if (mRunCase == FD_YUV_STREAM) {
    mbEnableNativeDevice = 1;
  }
  //
  {
    auto start = options.find("-s");
    if (start != std::string::npos) {
      auto tmpJsonPath = options.substr(start + 3);
      auto end = tmpJsonPath.find(" ");
      if (end != std::string::npos) {
        mJSONFilePath =
            std::string(JSON_FOLDER) + std::string(tmpJsonPath.substr(0, end));
      } else {
        mJSONFilePath = std::string(JSON_FOLDER) + std::string(tmpJsonPath);
      }
    } else {
      mJSONFilePath = std::string(JSON_FOLDER) + std::string("ut.json");
    }
  }
  MY_LOGI(
      "mTotalRequestNum %d, mRunCase:%d, mbEnableTuningData:%d, "
      "mbEnableNativeDevice:%d, JsonPath:%s",
      mTotalRequestNum, mRunCase, mbEnableTuningData, mbEnableNativeDevice,
      mJSONFilePath.c_str());
  FUNC_END
  return 0;
}

void MtkHalUt::allocateIonBuffer(Stream srcStream, StreamBuffer* dstBuffers) {
  FUNC_START

  MINT bufBoundaryInBytes[3] = {0};
  MUINT32 bufStridesInBytes[3] = {srcStream.width, srcStream.width / 2,
                                  srcStream.width / 2};

  size_t planeCount =
      NSCam::Utils::Format::queryPlaneCount(NSCam::eImgFmt_NV21);

  mcam::IImageBufferAllocator::ImgParam imgParam(
      srcStream.format, NSCam::MSize(srcStream.width, srcStream.height),
      srcStream.usage);
  //
#if 0
#define ALIGN_PIXEL(w, a) (((w + (a - 1)) / a) * a)
  switch (srcStream.format) {
    case NSCam::eImgFmt_MTK_YUV_P010:
    case NSCam::eImgFmt_MTK_YUV_P210:
      imgParam.bufStridesInBytes[0] = ALIGN_PIXEL(srcStream.width, 64) * 10 / 8;
      imgParam.bufStridesInBytes[1] = ALIGN_PIXEL(srcStream.width, 64) * 10 / 8;
      MY_LOGD("format:0x%X : stride is forced to 64 align with 10 bit : "
              "[%zu,%zu,%zu]",
        srcStream.format,
        imgParam.bufStridesInBytes[0],
        imgParam.bufStridesInBytes[1],
        imgParam.bufStridesInBytes[2]);
      break;
    case NSCam::eImgFmt_MTK_YUV_P012:
      imgParam.bufStridesInBytes[0] = ALIGN_PIXEL(srcStream.width, 64) * 12 / 8;
      imgParam.bufStridesInBytes[1] = ALIGN_PIXEL(srcStream.width, 64) * 12 / 8;
      MY_LOGD("format:0x%X : stride is forced to 64 align with 12 bit : "
              "[%zu,%zu,%zu]",
        srcStream.format,
        imgParam.bufStridesInBytes[0],
        imgParam.bufStridesInBytes[1],
        imgParam.bufStridesInBytes[2]);
      break;
    default:
      MY_LOGD("stride is decided by ImageBuffer");
      break;
  }
#undef ALIGN_PIXEL
#else
  if (srcStream.bufPlanes.planes[0].rowStrideInBytes == 0) {
    MY_LOGD("stride will be decided by ImageBuffer");
  } else {
    imgParam.strideInByte[0] = srcStream.bufPlanes.planes[0].rowStrideInBytes;
      imgParam.strideInByte[1] = srcStream.bufPlanes.planes[1].rowStrideInBytes;
      imgParam.strideInByte[2] = srcStream.bufPlanes.planes[2].rowStrideInBytes;
      MY_LOGD("ImageBuffer set stride [%d,%d,%d]", imgParam.strideInByte[0],
              imgParam.strideInByte[1], imgParam.strideInByte[2]);
  }
#endif
  //
  static int allocateBufSerialNo = 1;
   auto pImageBufferHeap = mcam::IImageBufferHeapFactory::create(
       LOG_TAG, "outStream", imgParam, true);
  MY_LOGA_IF(pImageBufferHeap == nullptr, "IIonImageBufferHeap::create fail");
  dstBuffers->streamId = srcStream.id;
  dstBuffers->bufferId = allocateBufSerialNo++;
  dstBuffers->status = mcam::BufferStatus::OK;
  dstBuffers->buffer = pImageBufferHeap;
  FUNC_END
}

int MtkHalUt::allocateStreamBuffers() {
  auto size = mLhsConfigOutputStreams.size();
  MY_LOGI("mLhsConfigOutputStreams size:%lu", size);
  mLhsAllocateOutputBuffers.resize(mMaxBuffers * size);
  size = mRhsConfigOutputStreams.size();
  mRhsAllocateOutputBuffers.resize(mMaxBuffers * size);
  mRhsAllocateInputBuffers.resize(mMaxBuffers);

  std::vector<int32_t> Ids;
  for (auto vec : mLhsReqs) {
    if (mRunCase == MCNR_F0_F1_ME && vec.reqOutput.id.size() == 3) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (mRunCase == MCNR_F0_F1 && vec.reqOutput.id.size() == 2) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (mRunCase == MCNR_F0 && vec.reqOutput.id.size() == 1) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (mRunCase == FD_YUV_STREAM && vec.reqOutput.id.size() == 1) {
      Ids = vec.reqOutput.id;
      break;
    }
    if (mRunCase == WPE_PQ && vec.reqOutput.id.size() == 1) {
      Ids = vec.reqOutput.id;
      break;
    }
  }
  MY_LOGD("Ids.size(%d)", Ids.size());
  for (auto num = 0; num < Ids.size(); num++) {
    MY_LOGD("num:%d", num);
    for (auto outStream : mLhsConfigOutputStreams) {
      MY_LOGD("outStream.id:%d Ids[num]:%d", outStream.id, Ids[num]);
      if (outStream.id == Ids[num]) {
        std::vector<StreamBuffer> outputBufs;
        outputBufs.resize(mMaxBuffers);
        for (int i = 0; i < mMaxBuffers; i++) {
          allocateIonBuffer(outStream, &mLhsAllocateOutputBuffers[i]);
          outputBufs[i] = mLhsAllocateOutputBuffers[i];
        }
        MY_LOGI("id:%d, w:%d, h:%d, size:%d", outStream.id, outStream.width,
                outStream.height, outStream.bufferSize);
        mLhsConfigOutputBufMap.emplace(outStream.id, outputBufs);
        break;
      }
    }
  }
  if (mbEnableNativeDevice) {
    for (auto out : mRhsConfigOutputStreams) {
      for (int i = 0; i < mMaxBuffers; i++) {
        allocateIonBuffer(out, &mRhsAllocateOutputBuffers[i]);
        MY_LOGD("allocateIonBuffer mRhsConfigOutputStreams, size:[%d,%d]",
                out.width, out.height);
        // auto const& buf = mRhsAllocateInputBuffers[i];
        // auto handle = (void *) (buf.buffer.getNativeHandle());
        // MY_LOGD("assign buffer handle:%p", handle);
      }
      mRhsConfigOutputBufMap.emplace(out.id, mRhsAllocateOutputBuffers);
    }
    // allocate warp map buffer (blob)
    if (mDeviceType == "WPE_PQ") {
      auto out = mRhsConfigInputStreams[1];
      for (int i = 0; i < mMaxBuffers; i++) {
        allocateIonBuffer(out, &mRhsAllocateInputBuffers[i]);
        MY_LOGD("allocateIonBuffer mRhsConfigInputStreams, size:[%d,%d]",
                out.width, out.height);
      }
      mRhsConfigInputBufMap.emplace(out.id, mRhsAllocateInputBuffers);
    }
  }
  FUNC_END
  return 0;
}

int MtkHalUt::queueRequest() {
  FUNC_START
  run("MtkHalUt::native");
  for (int i = 0; i < mMaxBuffers; i++) {
    //
    std::vector<StreamBuffer> tmp;
    int size = mLhsReqs[mLhsTargetNum].reqOutput.id.size();
    MY_LOGI("Lhs size:%d", size);
    tmp.resize(size);
    int bufNum = 0;
    for (auto id : mLhsReqs[mLhsTargetNum].reqOutput.id) {
      MY_LOGI("id:%d", id);
      auto bufHandl = mLhsConfigOutputBufMap.at(id);
      tmp[bufNum++] = bufHandl[i];
    }
    mLhsTotalAvailableBufMap.emplace(i, tmp);
    //
    tmp.clear();
    size = mRhsReqs[mRhsTargetNum].reqOutput.id.size();
    MY_LOGI("Rhs size:%d", size);
    tmp.resize(size);
    bufNum = 0;
    for (auto id : mRhsReqs[mRhsTargetNum].reqOutput.id) {
      MY_LOGI("id:%d", id);
      auto bufHandl = mRhsConfigOutputBufMap.at(id);
      tmp[bufNum++] = bufHandl[i];
    }
    mRhsTotalAvailableBufMap.emplace(i, tmp);
  }

  for (int i = 0; i < mTotalRequestNum; i++) {
    // don't sent all request in the same time, need to wait a while
    if (mbEnableNativeDevice) {
      std::unique_lock<std::mutex> _l(mRhsLock);
      if (i > 4) {  // mLhsTotalAvailableBufMap.empty()
        MY_LOGD("stat wait mRhsCondition (+)");
        auto timeOut =
            std::chrono::system_clock::now() + std::chrono::seconds(2);
        // mRhsCondition.wait_until(_l, timeOut);
        mRhsCondition.wait_until(_l, timeOut);
        MY_LOGD("stat wait mRhsCondition (-)");
      }
    } else {
      if (i > 4) {
        std::unique_lock<std::mutex> lk(mLhsLock);
        MY_LOGD("stat wait mLhsCondition (+)");
        mLhsCondition.wait(lk);
        MY_LOGD("stat wait mLhsCondition (-)");
        // move cached buffer map to total buffer map
        auto iter = mLhsEnquedBufMap.begin();
        mLhsTotalAvailableBufMap.emplace((iter->first + mMaxBuffers),
                                         iter->second);
        mLhsEnquedBufMap.erase(iter);
      }
    }
    //
    auto iter = mLhsTotalAvailableBufMap.begin();
    if (iter == mLhsTotalAvailableBufMap.end()) {
      MY_LOGA(
          "out of buffer (mLhsTotalAvailableBufMap), please check the flow");
    }
    int frameNumber = (i + 1);
    //
    uint32_t numRequestProcessed = 0;
    std::vector<mcam::CaptureRequest> mtkRequests;
    mcam::CaptureRequest oneRequest;
    oneRequest.frameNumber = frameNumber;
    // oneRequest.inputBuffers = ...
    oneRequest.outputBuffers = iter->second;
    oneRequest.settings = mLhsRequestSetting;
    // oneRequest.halSettings = ...
    // oneRequest.physicalCameraSettings = ...
    // oneRequest.subRequests = ...

    mtkRequests.push_back(oneRequest);
    //
    toLog("Lhs mtkRequests", mtkRequests);
    //
    int ret =
        mLhsSession->processCaptureRequest(mtkRequests, numRequestProcessed);
    MY_LOGI("numRequestProcessed:%d", numRequestProcessed);

    // move used buffer to cached buffer map
    mLhsEnquedBufMap.emplace(frameNumber, iter->second);
    mLhsTotalAvailableBufMap.erase(iter);
    {
      std::unique_lock<std::mutex> l(mLhsLock);
      //
      std::shared_ptr<InFlightRequest> inflightReq =
          std::make_shared<InFlightRequest>();
      //
      inflightReq->frameNumber = frameNumber;
      inflightReq->inputBufferNum = oneRequest.inputBuffers.size();
      inflightReq->outputBufferNum = oneRequest.outputBuffers.size();
      MY_LOGD(
          "LHS:inflightReq:frameNumber(%d) inputBufferNum(%d) "
          "outputBufferNum(%d)",
          frameNumber, inflightReq->inputBufferNum,
          inflightReq->outputBufferNum);
      //
      mLhsInflightMap.emplace(frameNumber, inflightReq);
      //
      MY_LOGI("RHS:frame:%d, frameNum:%ld, size:%zu", i,
              inflightReq->frameNumber, mLhsInflightMap.size());
    }
  }
  FUNC_END
  return 0;
}

int MtkHalUt::flush() {
  // auto ret =  mExtHandler->flush();
  int ret = 0;
  if (ret) {
    return 1;
  } else {
    return 0;
  }
}

auto MtkHalUt::readyToRun() -> android::status_t {
  return android::OK;
}

bool MtkHalUt::isReadyToExit() {
  FUNC_START
  if (mLhsInflightMap.size() == 0 && mRhsInflightMap.size() == 0) {
    MY_LOGD("ready to exit");
    FUNC_END
    return true;
  }
  MY_LOGD(
      "NOT ready to exit, "
      "mLhsInflightMap.size(%d), mRhsInflightMap.size(%d)",
      mLhsInflightMap.size(), mRhsInflightMap.size());
  FUNC_END
  return false;
}

auto MtkHalUt::requestExit() -> void {
  FUNC_START
  std::unique_lock<std::mutex> lk(mRhsLock);
  mRhsCondition.wait(lk, [this]() { return isReadyToExit(); });
  MY_LOGD("Ready to exit thread");
  Thread::requestExit();
  mLhsCondition.notify_all();
  FUNC_END
}
auto MtkHalUt::threadLoop() -> bool {
  FUNC_START
  // wait the ext interface done
  MY_LOGI("%s", __FUNCTION__);
  std::unique_lock<std::mutex> _l(mLhsLock);
  MY_LOGI("wait the condition");
  mLhsCondition.wait(_l, [this]() {
    return (!mLhsReceivedFrameNumber.empty() || exitPending());
  });

  if (isReadyToExit()) {
    FUNC_END
    return false;
  }
  MY_LOGI("In Processing...");

  if (mbEnableNativeDevice && !mLhsReceivedFrameNumber.empty()) {
    auto frameNumber = mLhsReceivedFrameNumber.front();
    mLhsReceivedFrameNumber.erase(mLhsReceivedFrameNumber.begin());
    // CameraMetadata appControl= mLhsInflightMap[frameNum]->collectedResult;
    auto it = mLhsInflightMap.find(frameNumber);
    auto inFlightRequest = it->second;
    // mLhsInflightMap.erase(it);
    // mLhsKeepTempInflightMap.emplace(frameNumber, inFlightRequest);
    // MY_LOGD("mLhsKeepTempInflightMap emplace frameNumber:%d",frameNumber);
    //
    // mRhsConfigInputStreams = mLhsConfigOutputStreams;

    ////////////////////////////////////////////////
    auto iter = mRhsTotalAvailableBufMap.begin();
    if (iter == mRhsTotalAvailableBufMap.end()) {
      MY_LOGA("mRhsTotalAvailableBufMap is empty!");
    }
    //
    uint32_t numRequestProcessed = 0;
    std::vector<mcam::CaptureRequest> mtkRequests;
    mcam::CaptureRequest oneRequest;
    oneRequest.frameNumber = frameNumber;
    oneRequest.inputBuffers = inFlightRequest->resultOutputBuffers;
    //
    if (mRunCase == MCNR_F0_F1_ME && frameNumber > 1) {
      auto pre_it = mLhsInflightMap.find(frameNumber - 1);
      if (pre_it == mLhsInflightMap.end()) {
        MY_LOGA(
            "can't find previous frameNumber(%d) InFlightRequest"
            " in MCNR_F0_F1_ME case",
            frameNumber - 1);
      }
      auto preInFlightRequest = pre_it->second;
      bool bFound = false;
      for (auto streamBuf : preInFlightRequest->resultOutputBuffers) {
        if (mMeStreamId == streamBuf.streamId) {
          MY_LOGD("found tME stream in preRequest");
          streamBuf.streamId = TME_STREAM_ID;
          oneRequest.inputBuffers.push_back(streamBuf);
          bFound = true;
          break;
        }
      }
      if (!bFound) {
        MY_LOGA(
            "can't found preRequest TME stream, "
            "preInFlightRequest resultOutputBuffers.size(%d)",
            preInFlightRequest->resultOutputBuffers.size());
      }
    } else if (mRunCase == WPE_PQ) {
      // Add warp map
      auto writeWarpMap = [](StreamBuffer& buf, int32_t imgW, int32_t imgH,
                             bool enableWarpMapCrop) {
        MY_LOGD("writeWarpMap start, imgW: %d, imgH: %d, enableWarpMapCrop: %d",
                imgW, imgH, enableWarpMapCrop);
        // write warp map into buffer
        auto& warpMapBuf = buf.buffer;
        warpMapBuf->lockBuf("WarpMap",
                            mcam::eBUFFER_USAGE_SW_READ_OFTEN |
                                mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                                mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE);
        int* pBufferVA_x = reinterpret_cast<int*>(warpMapBuf->getBufVA(0));
        int* pBufferVA_y = reinterpret_cast<int*>(warpMapBuf->getBufVA(1));

        // write identity map: (0,0) (w-1, 0) (0, h-1),  (w-1, h-1)
        if (!enableWarpMapCrop) {
          int32_t shift[2] = {0};
          int32_t step[2] = {0};
          int32_t OutputW = imgW;
          int32_t OutputH = imgH;
          int32_t WarpMapW = 2;
          int32_t WarpMapH = 2;

          step[0] = OutputW / (WarpMapW - 1);
          step[1] = OutputH / (WarpMapH - 1);
          MY_LOGD("write identity map start");

          for (int j = 0; j < WarpMapH; j++) {
            for (int i = 0; i < WarpMapW; i++) {
              pBufferVA_x[j * WarpMapW + i] =
                  (!shift[0] && !i) ? 0 : ((shift[0] + step[0] * i - 1) * 32);
              pBufferVA_y[j * WarpMapW + i] =
                  (!shift[1] && !j) ? 0 : ((shift[1] + step[1] * j - 1) * 32);

              MY_LOGD("warp map k:%d, X value: %d, Y value: %d",
                      j * WarpMapW + i, pBufferVA_x[j * WarpMapW + i],
                      pBufferVA_y[j * WarpMapW + i]);
            }
          }
        } else if (enableWarpMapCrop) {
          struct WPE_Parameter {
            unsigned int in_width;
            unsigned int in_height;
            unsigned int in_offset_x;
            unsigned int in_offset_y;
            unsigned int out_width;
            unsigned int out_height;
            unsigned int warp_map_width;
            unsigned int warp_map_height;
          };

          WPE_Parameter wpe;
          wpe.in_width = imgW;
          wpe.in_height = imgH;
          wpe.in_offset_x = 0;
          wpe.in_offset_y = 0;
          wpe.out_width =
              ::property_get_int32("vendor.debug.hal3ut.wpe.outputw", 720);
          wpe.out_height =
              ::property_get_int32("vendor.debug.hal3ut.wpe.outputh", 1280);
          wpe.warp_map_width = 2;
          wpe.warp_map_height = 2;

          if ((wpe.in_offset_x + wpe.out_width) > wpe.in_width) {
            MY_LOGF("Crop area is over input image width");
          } else if ((wpe.in_offset_y + wpe.out_height) > wpe.in_height) {
            MY_LOGF("Crop area is over input image height");
          }

          float stepX = static_cast<float>(wpe.out_width) /
                        static_cast<float>(wpe.warp_map_width - 1);
          float stepY = static_cast<float>(wpe.out_height) /
                        static_cast<float>(wpe.warp_map_height - 1);
          MY_LOGD("StepX = %f, StepY = %f", stepX, stepY);

          float oy = wpe.in_offset_y;
          float ox = 0;
          for (int i = 0; i < wpe.warp_map_height; i++) {
            ox = wpe.in_offset_x;
            for (int j = 0; j < wpe.warp_map_width; j++) {
              if (ox >= (wpe.in_offset_x + wpe.out_width))
                ox = wpe.in_offset_x + wpe.out_width - 1.0f;
              if (oy >= (wpe.in_offset_y + wpe.out_height))
                oy = wpe.in_offset_y + wpe.out_height - 1.0f;

              pBufferVA_x[i * wpe.warp_map_width + j] =
                  static_cast<int>(ox * 32.0f);
              pBufferVA_y[i * wpe.warp_map_width + j] =
                  static_cast<int>(oy * 32.0f);

              MY_LOGD("warp map k:%d, X value: %d, Y value: %d",
                      i * wpe.warp_map_width + j,
                      pBufferVA_x[i * wpe.warp_map_width + j],
                      pBufferVA_y[i * wpe.warp_map_width + j]);

              ox += stepX;
            }
            oy += stepY;
          }
        }
        warpMapBuf->unlockBuf("WarpMap");
      };

      auto& inputBuf = mRhsAllocateInputBuffers[frameNumber % mMaxBuffers];
      MY_LOGD("get warp map streamBuffer: id (%d) buf (%p)", inputBuf.streamId,
              inputBuf.buffer.get());
      bool enableWarpMapCrop =
          ::property_get_int32("vendor.debug.hal3ut.wpe.enableWarpMapCrop", 0);
      MY_LOGD("enableWarpMapCrop: %d", enableWarpMapCrop);
      auto imgSize = oneRequest.inputBuffers[0].buffer->getImgSize();
      writeWarpMap(inputBuf, imgSize.w, imgSize.h, enableWarpMapCrop);
      oneRequest.inputBuffers.push_back(inputBuf);
    }
    //
    oneRequest.outputBuffers = iter->second;
    oneRequest.settings = mRhsRequestSetting;
    oneRequest.halSettings = inFlightRequest->fullHalResult;
    // oneRequest.physicalCameraSettings = ...
    // oneRequest.subRequests = ...
    mtkRequests.push_back(oneRequest);
    //
    toLog("Rhs mtkRequests", mtkRequests);
    //
    int ret =
        mRhsSession->processCaptureRequest(mtkRequests, numRequestProcessed);
    MY_LOGI("numRequestProcessed:%d", numRequestProcessed);
    //
    mRhsEnquedBufMap.emplace(frameNumber, iter->second);
    mRhsTotalAvailableBufMap.erase(iter);
    {
      std::unique_lock<std::mutex> l(mRhsLock);
      //
      std::shared_ptr<InFlightRequest> inflightReq =
          std::make_shared<InFlightRequest>();
      //
      inflightReq->frameNumber = frameNumber;
      inflightReq->inputBufferNum = 0;
      inflightReq->outputBufferNum = oneRequest.outputBuffers.size();
      MY_LOGD(
          "RHS:inflightReq:frameNumber(%d) inputBufferNum(%d) "
          "outputBufferNum(%d)",
          frameNumber, inflightReq->inputBufferNum,
          inflightReq->outputBufferNum);
      //
      mRhsInflightMap.emplace(frameNumber, inflightReq);
      //
      MY_LOGI("RHS:frameNum:%ld, size:%zu", inflightReq->frameNumber,
              mRhsInflightMap.size());
    }
    ////////////////////////////////////////////////
  }
  FUNC_END
  return true;
}

void MtkHalUt::LhsNotify(const std::vector<mcam::NotifyMsg>& messages) {
  FUNC_START
  std::lock_guard<std::mutex> l(mLhsLock);
  MY_LOGI("LHS get notify");
  for (size_t i = 0; i < messages.size(); i++) {
    switch (messages[i].type) {
      case MsgType::ERROR:
        if (ErrorCode::ERROR_DEVICE == messages[i].msg.error.errorCode) {
          MY_LOGE("Camera reported serious device error");
        } else {
          auto it = mLhsInflightMap.find(messages[i].msg.error.frameNumber);
          if (it == mLhsInflightMap.end()) {
            MY_LOGE("Unexpected error frame number: %u",
                    messages[i].msg.error.frameNumber);
            break;
          }
          auto r = it->second;
          if (ErrorCode::ERROR_RESULT == messages[i].msg.error.errorCode &&
              messages[i].msg.error.errorStreamId != -1) {
            MY_LOGE("ERROR_RESULT! frame number: %u, stream id: %d",
                    messages[i].msg.error.frameNumber,
                    messages[i].msg.error.errorStreamId);
            break;

          } else {
            r->errorCodeValid = true;
            r->errorCode = messages[i].msg.error.errorCode;
            r->errorStreamId = messages[i].msg.error.errorStreamId;
          }
        }
        break;
      case MsgType::SHUTTER: {
        MY_LOGI("get the shutter");
        auto it = mLhsInflightMap.find(messages[i].msg.shutter.frameNumber);
        if (it == mLhsInflightMap.end()) {
          MY_LOGE("Unexpected shutter frame number! received: %u",
                  messages[i].msg.shutter.frameNumber);
          break;
        }
        auto r = it->second;
        r->shutterTimestamp = messages[i].msg.shutter.timestamp;
      } break;
      default:
        MY_LOGE("Unsupported notify message %d", messages[i].type);
        break;
    }
  }
  FUNC_END
}

bool MtkHalUt::processResult(const mcam::CaptureResult& oneResult,
                             InFlightMap& inflightMap,
                             std::string& msg) {
  FUNC_START
  MY_LOGD(
      "inputBuffers size:%d, outputBuffers size:%d, "
      "bLastPartialResult:%d, AppResult.size(%d), HalResult.size(%d)",
      oneResult.inputBuffers.size(), oneResult.outputBuffers.size(),
      oneResult.bLastPartialResult, oneResult.result.count(),
      oneResult.halResult.count());
  //
  auto frameNumber = oneResult.frameNumber;
  if ((oneResult.result.isEmpty()) && (oneResult.halResult.isEmpty()) &&
      (oneResult.outputBuffers.size() == 0) &&
      (oneResult.inputBuffers.size() == 0)) {
    MY_LOGE("No result data provided by HAL for frame %d", frameNumber);
    return false;
  }
  //
  auto it = inflightMap.find(frameNumber);
  if (it == inflightMap.end()) {
    MY_LOGE("Unexpected frame number! received: %u", frameNumber);
    return false;
  }
  auto& request = it->second;
  //
  if (oneResult.bLastPartialResult) {
    if (request->haveLastResultMetadata) {
      MY_LOGE("received multi bLastPartialResult!  frameNumber: %u",
              frameNumber);
      return false;
    } else {
      request->haveLastResultMetadata = true;
      MY_LOGD("frameNumber(%d) received bLastPartialResult", frameNumber);
    }
  }
  //
  // collect all metadata and output stream buffers
  if (!oneResult.result.isEmpty()) {
    request->fullAppResult += oneResult.result;
  }
  if (!oneResult.halResult.isEmpty()) {
    request->fullHalResult += oneResult.halResult;
  }
  if (oneResult.outputBuffers.size() != 0) {
    request->resultOutputBuffers.insert(request->resultOutputBuffers.end(),
                                        oneResult.outputBuffers.begin(),
                                        oneResult.outputBuffers.end());
  }
  //
  if (request->resultOutputBuffers.size() == request->outputBufferNum) {
    request->receivedOutputBuffer = true;
    MY_LOGD("frameNumber(%d) received outputBuffers", frameNumber);
  }
  // dump processed stream buffer
  bool notifyDone = false;
  if (request->receivedOutputBuffer && request->haveLastResultMetadata) {
    MY_LOGD("frameNumber(%d) received done!", frameNumber);
    notifyDone = true;
    //
    MY_LOGD(
        "resultOutputBuffers size:%d, "
        "fullAppResult.size(%d), fullHalResult.size(%d)",
        request->resultOutputBuffers.size(), request->fullAppResult.count(),
        request->fullHalResult.count());
  }
  //
  // dump processed stream buffer
  if (notifyDone) {
    for (auto& streamBuf : request->resultOutputBuffers) {
      dumpStreamBuffer(streamBuf, msg.c_str(), frameNumber);
    }
  }
  ////////////////////////////////////////////
  FUNC_END
  return notifyDone;
}

void MtkHalUt::LhsProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  FUNC_START
  MY_LOGD("results size:%d", results.size());
  for (size_t i = 0; i < results.size(); i++) {
    std::unique_lock<std::mutex> l(mLhsLock);
    //
    bool notifyDone = false;
    auto frameNumber = results[i].frameNumber;
    std::string msg = "LHS";
    notifyDone = processResult(results[i], mLhsInflightMap, msg);
    //
    if (notifyDone) {
      MY_LOGI("mLhsReceivedFrameNumber.push_back frameNumber:%d", frameNumber);
      mLhsReceivedFrameNumber.push_back(frameNumber);
    }
    l.unlock();
    if (notifyDone) {
      mLhsCondition.notify_all();
    }
  }
  FUNC_END
}

void MtkHalUt::RhsNotify(const std::vector<mcam::NotifyMsg>& messages) {
  FUNC_START
  std::lock_guard<std::mutex> l(mRhsLock);
  MY_LOGI("RHS get notify");
  for (size_t i = 0; i < messages.size(); i++) {
    switch (messages[i].type) {
      case MsgType::ERROR:
        if (ErrorCode::ERROR_DEVICE == messages[i].msg.error.errorCode) {
          MY_LOGE("Camera reported serious device error");
        } else {
          auto it = mRhsInflightMap.find(messages[i].msg.error.frameNumber);
          if (it == mRhsInflightMap.end()) {
            MY_LOGE("Unexpected error frame number: %u",
                    messages[i].msg.error.frameNumber);
            break;
          }
          auto r = it->second;
          if (ErrorCode::ERROR_RESULT == messages[i].msg.error.errorCode &&
              messages[i].msg.error.errorStreamId != -1) {
            MY_LOGE("ERROR_RESULT! frame number: %u, stream id: %d",
                    messages[i].msg.error.frameNumber,
                    messages[i].msg.error.errorStreamId);
            break;

          } else {
            r->errorCodeValid = true;
            r->errorCode = messages[i].msg.error.errorCode;
            r->errorStreamId = messages[i].msg.error.errorStreamId;
          }
        }
        break;
      case MsgType::SHUTTER: {
        MY_LOGI("get the shutter");
        auto it = mRhsInflightMap.find(messages[i].msg.shutter.frameNumber);
        if (it == mRhsInflightMap.end()) {
          MY_LOGE("Unexpected shutter frame number! received: %u",
                  messages[i].msg.shutter.frameNumber);
          break;
        }
        auto r = it->second;
        r->shutterTimestamp = messages[i].msg.shutter.timestamp;
      } break;
      default:
        MY_LOGE("Unsupported notify message %d", messages[i].type);
        break;
    }
  }
  FUNC_END
}

void MtkHalUt::RhsProcessCompleted(
    const std::vector<mcam::CaptureResult>& results) {
  FUNC_START
  MY_LOGD("results size:%d", results.size());
  for (size_t i = 0; i < results.size(); i++) {
    std::unique_lock<std::mutex> l(mRhsLock);
    //
    bool notifyDone = false;
    auto frameNumber = results[i].frameNumber;
    std::string msg = "RHS";
    notifyDone = processResult(results[i], mRhsInflightMap, msg);
    //
    if (notifyDone) {
      MY_LOGI("frameNumber:%d", frameNumber);
      mRhsReceivedFrameNumber.push_back(frameNumber);
      //
      auto it_rhsEnque = mRhsEnquedBufMap.find(frameNumber);
      if (it_rhsEnque != mRhsEnquedBufMap.end()) {
        auto last_it = mRhsTotalAvailableBufMap.rbegin();
        if (last_it != mRhsTotalAvailableBufMap.rend()) {
          auto last_id = last_it->first;
          MY_LOGD("return buffer to mRhsTotalAvailableBufMap (id:%d)",
                  last_id + 1);
          mRhsTotalAvailableBufMap.emplace(last_id + 1, it_rhsEnque->second);
          MY_LOGD("return buffer from mRhsEnquedBufMap (frameNumber:%d)",
                  last_id + 1);
          mRhsEnquedBufMap.erase(it_rhsEnque);
        } else {
          MY_LOGA("mRhsTotalAvailableBufMap is empty!");
        }
      } else {
        MY_LOGA("mRhsEnquedBufMap not found frameNumber:%d", frameNumber);
      }
      //
      auto it_inFlight = mRhsInflightMap.find(frameNumber);
      if (it_inFlight != mRhsInflightMap.end()) {
        MY_LOGD("erase mRhsInflightMap frameNumber:%d", frameNumber);
        mRhsInflightMap.erase(it_inFlight);
      } else {
        MY_LOGA("mRhsInflightMap not found frameNumber:%d", frameNumber);
      }
      //
      if (mRunCase == MCNR_F0_F1_ME && frameNumber > 1) {
        auto it_enqued = mLhsEnquedBufMap.find(frameNumber - 1);
        if (it_enqued != mLhsEnquedBufMap.end()) {
          auto last_it = mLhsTotalAvailableBufMap.rbegin();
          if (last_it != mLhsTotalAvailableBufMap.rend()) {
            auto last_id = last_it->first;
            MY_LOGD("return buffer to mLhsTotalAvailableBufMap (id:%d)",
                    last_id + 1);
            mLhsTotalAvailableBufMap.emplace(last_id + 1, it_enqued->second);
            MY_LOGD("return buffer from mLhsEnquedBufMap (frameNumber:%d)",
                    last_id + 1);
            mLhsEnquedBufMap.erase(it_enqued);
          } else {
            MY_LOGA("mLhsTotalAvailableBufMap is empty!");
          }
        } else {
          MY_LOGA("mLhsEnquedBufMap not found frameNumber:%d", frameNumber - 1);
        }
        //
        auto it_inFlight = mLhsInflightMap.find(frameNumber - 1);
        if (it_inFlight != mLhsInflightMap.end()) {
          MY_LOGD("erase mLhsInflightMap frameNumber:%d", frameNumber - 1);
          mLhsInflightMap.erase(it_inFlight);
        } else {
          MY_LOGA("mLhsInflightMap not found frameNumber:%d", frameNumber - 1);
        }
        //
        if (frameNumber == mTotalRequestNum) {
          auto it_enqued = mLhsEnquedBufMap.find(frameNumber);
          if (it_enqued != mLhsEnquedBufMap.end()) {
            auto last_it = mLhsTotalAvailableBufMap.rbegin();
            if (last_it != mLhsTotalAvailableBufMap.rend()) {
              auto last_id = last_it->first;
              MY_LOGD("return buffer to mLhsTotalAvailableBufMap (id:%d)",
                      last_id + 1);
              mLhsTotalAvailableBufMap.emplace(last_id + 1, it_enqued->second);
              MY_LOGD("return buffer from mLhsEnquedBufMap (frameNumber:%d)",
                      last_id + 1);
              mLhsEnquedBufMap.erase(it_enqued);
            } else {
              MY_LOGA("mLhsTotalAvailableBufMap is empty!");
            }
          } else {
            MY_LOGA("mLhsEnquedBufMap not found frameNumber:%d", frameNumber);
          }
          //
          auto it_inFlight = mLhsInflightMap.find(frameNumber);
          if (it_inFlight != mLhsInflightMap.end()) {
            MY_LOGD("erase mLhsInflightMap frameNumber:%d", frameNumber);
            mLhsInflightMap.erase(it_inFlight);
          } else {
            MY_LOGA("mLhsInflightMap not found frameNumber:%d", frameNumber);
          }
        }
      } else if (mRunCase != MCNR_F0_F1_ME) {
        auto it_enqued = mLhsEnquedBufMap.find(frameNumber);
        if (it_enqued != mLhsEnquedBufMap.end()) {
          auto last_it = mLhsTotalAvailableBufMap.rbegin();
          if (last_it != mLhsTotalAvailableBufMap.rend()) {
            auto last_id = last_it->first;
            MY_LOGD("return buffer to mLhsTotalAvailableBufMap (id:%d)",
                    last_id + 1);
            mLhsTotalAvailableBufMap.emplace(last_id + 1, it_enqued->second);
            MY_LOGD("return buffer from mLhsEnquedBufMap (frameNumber:%d)",
                    last_id + 1);
            mLhsEnquedBufMap.erase(it_enqued);
          } else {
            MY_LOGA("mLhsTotalAvailableBufMap is empty!");
          }
        } else {
          MY_LOGA("mLhsEnquedBufMap not found frameNumber:%d", frameNumber);
        }
        //
        auto it_inFlight = mLhsInflightMap.find(frameNumber);
        if (it_inFlight != mLhsInflightMap.end()) {
          MY_LOGD("erase mLhsInflightMap frameNumber:%d", frameNumber);
          mLhsInflightMap.erase(it_inFlight);
        } else {
          MY_LOGA("mLhsInflightMap not found frameNumber:%d", frameNumber);
        }
      }
    }
    l.unlock();
    if (notifyDone) {
      mRhsCondition.notify_all();
    }
  }
  FUNC_END
}
void MtkHalUt::dumpStreamBuffer(const StreamBuffer& streamBuf,
                                std::string prefix_msg,
                                uint32_t frameNumber) {
  FUNC_START
  auto streamId = streamBuf.streamId;
  auto pImgBufHeap = streamBuf.buffer;
  auto imgBitsPerPixel = pImgBufHeap->getImgBitsPerPixel();
  auto imgSize = pImgBufHeap->getImgSize();
  auto imgFormat = pImgBufHeap->getImgFormat();
  auto imgStride = pImgBufHeap->getBufStridesInBytes(0);
  std::string extName;
  //
  if (imgFormat == NSCam::eImgFmt_JPEG) {
    extName = "jpg";
  } else if (imgFormat == NSCam::eImgFmt_RAW16 ||
             (imgFormat >= 0x2200 && imgFormat <= 0x2300)) {
    extName = "raw";
  } else {
    extName = "yuv";
  }
  //
  char szFileName[1024];
  snprintf(szFileName, sizeof(szFileName), "%s%s-PW_%d_%dx%d_%d_%d.%s",
           DUMP_FOLDER, prefix_msg.c_str(), frameNumber, imgSize.w, imgSize.h,
           imgStride, streamId, extName.c_str());
  //
  auto imgBuf = pImgBufHeap->createImageBuffer();
  //
  MY_LOGD("dump image frameNum(%d) streamId(%d) to %s", frameNumber, streamId,
          szFileName);
  imgBuf->lockBuf(prefix_msg.c_str());
  imgBuf->saveToFile(szFileName);
  imgBuf->unlockBuf(prefix_msg.c_str());
  //
  FUNC_END
}

// CallbackListener Interface
/******************************************************************************
 *
 ******************************************************************************/
MtkHalUt::CallbackListener::CallbackListener(MtkHalUt* pParent, bool bIsLhs) {
  mParent = pParent;
  mbIsLhs = bIsLhs;
}

/******************************************************************************
 *
 ******************************************************************************/
auto MtkHalUt::CallbackListener::create(MtkHalUt* pParent, bool bIsLhs)
    -> CallbackListener* {
  auto pInstance = new CallbackListener(pParent, bIsLhs);
  return pInstance;
}

/******************************************************************************
 *
 ******************************************************************************/
void MtkHalUt::CallbackListener::processCaptureResult(
    const std::vector<mcam::CaptureResult>& results) {
  if (mbIsLhs)
    mParent->LhsProcessCompleted(results);
  else
    mParent->RhsProcessCompleted(results);
}

/******************************************************************************
 *
 ******************************************************************************/
void MtkHalUt::CallbackListener::notify(
    const std::vector<mcam::NotifyMsg>& msgs) {
  if (mbIsLhs)
    mParent->LhsNotify(msgs);
  else
    mParent->RhsNotify(msgs);
}
