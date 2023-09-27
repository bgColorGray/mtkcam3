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
#define LOG_TAG "MtkCam/CoreDeviceWPE"

#include "coredev/CoreDeviceWPEImpl.h"
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include "mtkcam/utils/debug/Properties.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mtkcam/drv/iopipe/PostProc/IWpeStream.h>
#include <mtkcam/drv/iopipe/PostProc/WpeUtility.h>
#include <mtkcam/drv/iopipe/Port.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_HAL_SERVER);

#define Core_OK 0
#define Not_OK -1

using std::make_shared;
using std::move;
using std::shared_ptr;
using std::vector;
using NSCam::Utils::Format::queryPlaneWidthInPixels;
using NSCam::Utils::Format::queryPlaneBitsPerPixel;
using namespace NSCam::NSIoPipe::NSWpe;

#define WDMAO_CROP_GROUP 2
#define WROTO_CROP_GROUP 3
#define DUMP_FOLDER "/data/vendor/camera_dump/"

namespace NSCam {
namespace v3 {
namespace CoreDev {
namespace model {

#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)

#define ALIGN(x, alignment) (((x) + ((alignment)-1)) & ~((alignment)-1))

static bool toDpColorFormat(const NSCam::EImageFormat fmt, DpColorFormat &dpFmt)
{
    bool ret = true;
    switch( fmt )
    {
        case eImgFmt_YUY2:    dpFmt = DP_COLOR_YUYV;      break;
        case eImgFmt_NV21:    dpFmt = DP_COLOR_NV21;      break;
        case eImgFmt_NV12:    dpFmt = DP_COLOR_NV12;      break;
        case eImgFmt_YV12:    dpFmt = DP_COLOR_YV12;      break;
        default:
            MY_LOGE("fmt(0x%x) not support in DP", fmt);
            ret = false;
            break;
    }
    return ret;
}
static MVOID* mainThreadLoop(MVOID* arg) {
  CoreDeviceWPEImpl* impl = reinterpret_cast<CoreDeviceWPEImpl*>(arg);
  MY_LOGD("threadloop +");

  while (!impl->mStop) {
    shared_ptr<RequestData> req = nullptr;
    if (0 == impl->onDequeRequest(req) && req != nullptr)
    {
        if (impl->processReq(req) != 0)
          MY_LOGE("request error");
    }
  }

  MY_LOGD("threadloop -");
  return nullptr;
}

auto CoreDeviceWPEImpl::onDequeRequest(shared_ptr<RequestData>& req)
    -> int32_t {
  MY_LOGD_IF(mLogLevel, "+");

  Mutex::Autolock _l(mReqQueueLock);
  if (mvReqQueue.empty())
  {
      mRequestQueueCond.wait(mReqQueueLock);
  }
  if (!mvReqQueue.empty())
  {
      req = mvReqQueue.front();
      mvReqQueue.pop_front();
  }

  MY_LOGD_IF(mLogLevel, "-");
  return Core_OK;
}

CoreDeviceWPEImpl::~CoreDeviceWPEImpl() {
  MY_LOGI("destroy CoreDeviceWPEImpl +");

  MY_LOGI("destroy CoreDeviceWPEImpl -");
}
auto
CoreDeviceWPEImpl::
uninit(
) -> int32_t
{
  MY_LOGD("uninit +");
  if (mInited)
  {
      MY_LOGD("uninit +1");
      mStop = true;
      mRequestQueueCond.broadcast();
      pthread_join(mMainThread, NULL);
  }
  MY_LOGD("uninit +2");
  mInited = false;
  {
      Mutex::Autolock _l(mReqQueueLock);
      if (!mvReqQueue.empty())
      {
          MY_LOGE("request queue is not empty, something wrong!!");
          mvReqQueue.clear();
      }
  }
  MY_LOGD("uninit +3");
  if (mpStream != nullptr)
  {
      mpStream->uninit("CoreDeviceWPEImpl", NSCam::NSIoPipe::EStreamPipeID_WarpEG);
      mpStream->destroyInstance();
      mpStream = nullptr;
  }
  MY_LOGD("uninit +4");
  if (mpDpIspStream)
  {
      delete mpDpIspStream;
      mpDpIspStream = nullptr;
  }
  MY_LOGD("uninit +5");

  MY_LOGD("unint -");
  return Core_OK;
}

CoreDeviceWPEImpl::CoreDeviceWPEImpl(int32_t id) : mId(id) {
  MY_LOGI("create CoreDeviceWPEImpl");
}

static auto convertImageParam(mcam::IImageBufferAllocator::ImgParam const& imgParam, size_t planeCount)
    -> NSCam::IImageBufferAllocator::ImgParam {
  MUINT32 format = imgParam.format;
  auto isValid = [](const size_t val[]) -> bool {
    size_t sum = 0;
    for (int i = 0; i < 3; i++)
      sum += val[i];
    return (sum > 0);
  };
  // 1. handle BLOB or JPEG
  if (imgParam.format == NSCam::eImgFmt_BLOB)
    return NSCam::IImageBufferAllocator::ImgParam(imgParam.size.w, 0);
  if (imgParam.format == NSCam::eImgFmt_JPEG) {
    NSCam::IImageBufferAllocator::ImgParam param(imgParam.size, imgParam.bufSizeInByte[0], 0);
    param.bufCustomSizeInBytes[0] = imgParam.bufSizeInByte[0];
    param.imgSize = imgParam.size;  // we should get (w, h) for JPEG getImgSize
    return param;
  }
  // 2. handle other formats
  // 2.1 calculate stride (in byte)
  size_t strides[3] = {0};
  if (isValid(imgParam.strideInByte)) {
    ::memcpy(strides, imgParam.strideInByte, sizeof(strides));
  } else {
    size_t w_align = (imgParam.strideAlignInPixel)
                         ? ALIGN(imgParam.size.w, imgParam.strideAlignInPixel)
                         : imgParam.size.w;
    for (int i = 0; i < planeCount; i++) {
      strides[i] = queryPlaneWidthInPixels(format, i, w_align);
      // cal into byte
      strides[i] = (strides[i] * queryPlaneBitsPerPixel(format, i) + 7) / 8;
      // byte align stride
      if (imgParam.strideAlignInByte)
        strides[i] = ALIGN(strides[i], imgParam.strideAlignInByte);
    }
  }
  size_t boundary[3] = {0};  // this is phased out field
  if (isValid(imgParam.bufSizeInByte))
    return NSCam::IImageBufferAllocator::ImgParam(format, imgParam.size, strides, boundary,
                                                  imgParam.bufSizeInByte, planeCount);
  else
    return NSCam::IImageBufferAllocator::ImgParam(format, imgParam.size, strides, boundary,
                                                  planeCount);
}

auto CoreDeviceWPEImpl::convertImageBufferHeap(std::shared_ptr<mcam::IImageBufferHeap> pMcamHeap)
    -> ::android::sp<NSCam::IImageBufferHeap> {
  ::android::sp<NSCam::IImageBufferHeap> pHeap = nullptr;

  auto pGraphicImageBufferHeap = mcam::IGraphicImageBufferHeap::castFrom(pMcamHeap.get());
  if (pGraphicImageBufferHeap != nullptr) { // IGraphic
    pHeap = NSCam::IGraphicImageBufferHeap::create(
            pGraphicImageBufferHeap->getCallerName().c_str(), // new
            pGraphicImageBufferHeap->getGrallocUsage(),
            pGraphicImageBufferHeap->getImgSize(),
            pGraphicImageBufferHeap->getImgFormat(),
            pGraphicImageBufferHeap->getBufferHandlePtr(),
            pGraphicImageBufferHeap->getAcquireFence(),
            pGraphicImageBufferHeap->getReleaseFence(),
            pGraphicImageBufferHeap->getStride(),             // new
            pGraphicImageBufferHeap->getSecType());

    pHeap->setColorArrangement(pGraphicImageBufferHeap->getColorArrangement());
  } else {  // Ion
    pMcamHeap->lockBuf(LOG_TAG);
    MY_LOGD("create IonHeap+, planeCount %zu, heapCount %zu", pMcamHeap->getPlaneCount(), pMcamHeap->getHeapIDCount());
    mcam::IImageBufferAllocator::ImgParam mcamImgParam(pMcamHeap->getImgFormat(), pMcamHeap->getImgSize(), 0);
    NSCam::IImageBufferAllocator::ImgParam imgParam = convertImageParam(mcamImgParam, pMcamHeap->getPlaneCount());
    NSCam::IIonImageBufferHeap::AllocExtraParam extraParam;
    extraParam.vIonFd.resize(pMcamHeap->getHeapIDCount());
    for (size_t i = 0 ; i < pMcamHeap->getHeapIDCount() ; i++) {
      auto& ionFd = extraParam.vIonFd[i];
      if (i > 0 && pMcamHeap->getHeapID(i) == pMcamHeap->getHeapID(i-1)) {
        extraParam.contiguousPlanes = MTRUE;
        ionFd = extraParam.vIonFd[i-1];
      } else {
        ionFd = ::dup(pMcamHeap->getHeapID(i));
      }
      MY_LOGD("dup fd(%zu): %d -> %d", i, pMcamHeap->getHeapID(i), ionFd);
    }
    pMcamHeap->unlockBuf(LOG_TAG);
    pHeap = ::android::sp<NSCam::IIonImageBufferHeap>(
              NSCam::IIonImageBufferHeap::create("mtkcam_hal/custom", imgParam, extraParam, MTRUE));

  }
  MY_LOGD("create IonHeap-, pHeap %p", pHeap.get());
  return pHeap;
}

auto CoreDeviceWPEImpl::configureCore(InternalStreamConfig const& config)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("configure + ");
  mLogLevel = NSCam::Utils::Properties::property_get_int32(
            "vendor.debug.camera.coredevice.wpe.dump", 0);
  MY_LOGD("logLevel: %d", mLogLevel);
  if (mLogLevel > 0) {
    mOpenLog = true;
    dumpWPE = true;
  }
  if (mLogLevel == 2) {
    dumpImage = true;
  }
  if (config.SensorIdSet.size() == 0) {
    MY_LOGE("config.SensorIdSet.size() == 0, not support");
    return Not_OK;
  }

  uint32_t src_image_w, src_image_h;
  for (auto&& it : config.vInputInfo) {
    if (it.first < eSTREAMID_END_OF_APP) {
      if (it.second->getImgFormat() == eImgFmt_WARP_2PLANE) {
        MY_LOGD("warp map (%#" PRIx64 ") %dx%d", it.first,
                it.second->getImgSize().w, it.second->getImgSize().h);
        streamID_warpMap = it.first;
        continue;
      }
      streamID_srcImage = it.first;
      src_image_w = it.second->getImgSize().w;
      src_image_h = it.second->getImgSize().h;
      MY_LOGD("srcImage stream(%#" PRIx64 ") %dx%d is configured, format(%d)",
        it.first, src_image_w, src_image_h, it.second->getImgFormat());
    }
  }

  mPurePQDIP = (streamID_warpMap == -1);
  MY_LOGD("PurePQDIP: %d", mPurePQDIP);

  if (mPurePQDIP && mpDpIspStream == nullptr)
  {
    mpDpIspStream = new DpIspStream(DpIspStream::ISP_ZSD_STREAM);
    if (mpDpIspStream == nullptr)
    {
        MY_LOGE("new MDP module failed");
        return Not_OK;
    }
  } else if (mpStream == nullptr)
  {
      mpStream = NSCam::NSIoPipe::NSPostProc::INormalStream::createInstance(0xFFFF);
      if (mpStream == nullptr)
      {
          MY_LOGE("new WPE module failed");
          return Not_OK;
      }
      mpStream->init("HW_Handler_WPE", NSCam::NSIoPipe::EStreamPipeID_WarpEG);
  }

  mInited = true;
  mvReqQueue.clear();
  pthread_create(&mMainThread, NULL, mainThreadLoop, this);
  pthread_setname_np(mMainThread, "PostProc@WPE");

  MY_LOGD("configure - ");
  return Core_OK;
}

auto
CoreDeviceWPEImpl::
processReq(
    shared_ptr<RequestData> req
) -> int32_t
{
  // WPE
  if (!mPurePQDIP) {
    if (convertReqToNormalStream(req) != Core_OK)
      MY_LOGE("convert req %d fail", req->mRequestNo);
  }
  // MDP
  if (mPurePQDIP) {
    if (convertReqToDpIspStream(req) != Core_OK)
      MY_LOGE("convert req %d fail", req->mRequestNo);
  }

  return Core_OK;
}

auto CoreDeviceWPEImpl::processReqCore(int32_t reqNo,
                                          vector<shared_ptr<RequestData>> reqs)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD_IF(mOpenLog, "processReqCore + ");
  int err = Core_OK;

  for (size_t r = 0; r < reqs.size(); ++r) {
    MY_LOGD_IF(mOpenLog, "Push request %d into mvReqQueue (reqs size: %d)",
                          reqNo, reqs.size());
    auto& req = reqs[r];
    mvReqQueue.push_back(req);
    mRequestQueueCond.broadcast();
  }

  MY_LOGD_IF(mOpenLog, "processReqCore - ");
  return err;
}

auto
CoreDeviceWPEImpl::
convertReqToNormalStream(
    shared_ptr<RequestData>& req
) -> int32_t {
  MY_LOGD_IF(mOpenLog, "convertReqToNormalStream +, req: %d", req->mRequestNo);
  QParams enQParam;
  FrameParams frameParam;
  WPEQParams wpeParam;
  ExtraParam exParams;

  int64_t WPEOutSize[2] = {0};
  auto reqAppMeta = (req->mpAppControl).get();
  uint32_t setWDMAO = 0, setWROTO = 0;
  // struct timeval diff, curr, target;
  vector<::android::sp<NSCam::IImageBufferHeap>> tmpNSCamBufHeaps;
  vector<shared_ptr<mcam::IImageBufferHeap>> tmpmcamBufHeaps;

  wpeParam.wpe_mode = NSIoPipe::NSWpe::WPE_MODE_MDP;//fixed: DL mode
  wpeParam.vgen_hmg_mode = 0; // No warpmap_z

  MCrpRsInfo vgenCropInfo;//no use, but need to push one into mwVgenCropInfo
  vgenCropInfo.mCropRect.p_integral.x = 0;
  vgenCropInfo.mCropRect.p_integral.y = 0;
  vgenCropInfo.mCropRect.p_fractional.x = 0;
  vgenCropInfo.mCropRect.p_fractional.y = 0;
  wpeParam.mwVgenCropInfo.push_back(vgenCropInfo);

  // set input image
  {
    auto InBuffer = req->mInBufs.find(streamID_srcImage);
    if (InBuffer != req->mInBufs.end()) {
      tmpmcamBufHeaps.push_back(InBuffer->second);
      auto NSCamBufferHeap = convertImageBufferHeap(InBuffer->second);
      tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
      auto buf = NSCamBufferHeap->createImageBuffer();
      buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

      NSCam::NSIoPipe::Input input;
      input.mPortID = PORT_WPEI;
      input.mBuffer = buf;
      input.mPortID.group=0;
      frameParam.mvIn.push_back(input);

      MY_LOGD_IF(mOpenLog, "srcImage (%#" PRIx64 ") %dx%d stride:%d",
        streamID_srcImage,
        InBuffer->second->getImgSize().w, InBuffer->second->getImgSize().h,
        InBuffer->second->getBufStridesInBytes(0));
    } else {
      MY_LOGE("No input buffer");
    }
  }
  //set warp map
  {
    auto InBuffer = req->mInBufs.find(streamID_warpMap);
    if (InBuffer != req->mInBufs.end()) {
      tmpmcamBufHeaps.push_back(InBuffer->second);
      auto NSCamBufferHeap = convertImageBufferHeap(InBuffer->second);
      tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
      auto buf = NSCamBufferHeap->createImageBuffer();
      buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

      MY_LOGD_IF(mOpenLog, "warpMap (%#" PRIx64 ") %dx%d stride:%d",
        streamID_warpMap,
        InBuffer->second->getImgSize().w, InBuffer->second->getImgSize().h,
        InBuffer->second->getBufStridesInBytes(0));

      for (int i = 0; i < 2; i++) {
        WarpMatrixInfo *WarpMap = nullptr;
        if(i == 0){
            WarpMap = &(wpeParam.warp_veci_info);
        } else {
            WarpMap = &(wpeParam.warp_vec2i_info);
        }
        WarpMap->width = buf->getImgSize().w;
        WarpMap->height = buf->getImgSize().h;
        WarpMap->stride = buf->getBufStridesInBytes(0);//WarpMap->width*4;
        WarpMap->virtAddr = buf->getBufVA(i);
        WarpMap->phyAddr = buf->getBufPA(i);
        WarpMap->bus_size = NSCam::NSIoPipe::NSWpe::WPE_BUS_SIZE_32_BITS;
        WarpMap->addr_offset = 0;
        WarpMap->veci_v_flip_en = 0;
        MY_LOGD("warp_veci_info(i:%d) w: %d,h: %d,s: %d,VA: %p, PA: %p",
          i, WarpMap->width, WarpMap->height, WarpMap->stride, WarpMap->virtAddr, WarpMap->phyAddr);
      }

      if (dumpWPE) {
        auto warpBuffer = buf;
        int* pBufferVA_x = reinterpret_cast<int*>(warpBuffer->getBufVA(0));
        int* pBufferVA_y = reinterpret_cast<int*>(warpBuffer->getBufVA(1));
        auto sizeInPixel = InBuffer->second->getImgSize().w
                                          * InBuffer->second->getImgSize().h;
        for ( int i = 0; i < sizeInPixel; ++i ) {
          auto tmpValue = pBufferVA_x[i];
          MY_LOGD_IF(mOpenLog, "warpMap x(int) i: %d, value: %d", i, tmpValue);
        }
        for ( int i = 0; i < sizeInPixel; ++i ) {
          auto tmpValue = pBufferVA_y[i];
          MY_LOGD_IF(mOpenLog, "warpMap y(int) i: %d, value: %d", i, tmpValue);
        }
      }
    } else {
      MY_LOGE("No warp map buffer");
    }
  }

  // get wpe output size
  if (IMetadata::getEntry<int64_t>(reqAppMeta,
      MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, WPEOutSize[0], 0) == true) {
    IMetadata::getEntry<int64_t>(reqAppMeta,
            MTK_POSTPROCDEV_WPEPQ_WPE_OUTPUT_SIZE, WPEOutSize[1], 1);
    MY_LOGD_IF(mOpenLog, "WPEPQ_WPE_OUTPUT_SIZE: (%lldx%lld)",
              WPEOutSize[0], WPEOutSize[1]);

    wpeParam.wpecropinfo.x_start_point = 0;
    wpeParam.wpecropinfo.y_start_point = 0;
    wpeParam.wpecropinfo.x_end_point = WPEOutSize[0] - 1;
    wpeParam.wpecropinfo.y_end_point = WPEOutSize[1] - 1;
  }

  // obtain crop info from vendor tag
  int32_t crop[4] = {0};
  {
    IMetadata::IEntry entry =
                      reqAppMeta->entryFor(MTK_POSTPROCDEV_WPEPQ_CROP_INFO);
    for (int i = 0; i < entry.count(); i++) {
      crop[i] = entry.itemAt(i, Type2Type<int32_t>());
    }
    MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_CROP_INFO, [%d,%d,%d,%d]",
        crop[0], crop[1], crop[2], crop[3]);
    if (entry.count() == 0) {
      MY_LOGD_IF(mOpenLog, "getEntry MTK_POSTPROCDEV_WPEPQ_CROP_INFO Fail");
      if (WPEOutSize[0] != 0 && WPEOutSize[1] != 0) {
          crop[2] = WPEOutSize[0];
          crop[3] = WPEOutSize[1];
          MY_LOGD_IF(mOpenLog, "replace crop info with WPE out size: (%d.%d)",
                     crop[2], crop[3]);
      } else {
        crop[2] = req->mInBufs.find(streamID_srcImage)->second->getImgSize().w;
        crop[3] = req->mInBufs.find(streamID_srcImage)->second->getImgSize().h;
        MY_LOGD_IF(mOpenLog, "replace crop info with src image size: (%d.%d)",
                crop[2], crop[3]);
      }
    }
  }

  // obtain flip and rotate info from vendor tag
  uint8_t flip = 0;
  if (IMetadata::getEntry<uint8_t>(reqAppMeta,
      MTK_POSTPROCDEV_WPEPQ_FLIP, flip) != true) {
      MY_LOGD_IF(mOpenLog, "getEntry Fail: MTK_POSTPROCDEV_WPEPQ_FLIP"); }
  MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_FLIP, [%d]", flip);

  uint8_t rotate = 0;
  if (IMetadata::getEntry<uint8_t>(reqAppMeta,
      MTK_POSTPROCDEV_WPEPQ_ROTATE, rotate) != true) {
      MY_LOGD_IF(mOpenLog, "getEntry Fail: MTK_POSTPROCDEV_WPEPQ_ROTATE"); }
  MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_ROTATE, [%d]", rotate);

  int trans_sel = 0;
  if (flip == 1) {            // flip vertical
    trans_sel = 2;
  } else if (flip == 2) {     // flip horizontal
    trans_sel = 1;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_90) {
    trans_sel = 4;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_180) {
    trans_sel = 3;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_270) {
    trans_sel = 7;
  }
  MY_LOGD_IF(mOpenLog, "dst.mTransform: %d", trans_sel);

  // set destination
  int64_t dstID = -1, dst2ID = -1;
  MY_LOGD_IF(mOpenLog, "output buffer size: %d", req->mOutBufs.size());
  if (req->mOutBufs.size() > 2) {
    MY_LOGE("Not support more than 2 output buffer");
    return Not_OK;
  } else if (req->mOutBufs.size() == 2 && rotate != 0) {
    MY_LOGE("Not support 2 output buffer with rotate");
    return Not_OK;
  }
  for (auto&& it : req->mOutBufs){
    if (dstID < 0) {
      dstID = it.first;
      MY_LOGD_IF(mOpenLog, "dstID (%d) %dx%d", it.first,
            it.second->getImgSize().w, it.second->getImgSize().h);
    } else {
      dst2ID = it.first;
      MY_LOGD_IF(mOpenLog, "dst2ID (%d) %dx%d", it.first,
            it.second->getImgSize().w, it.second->getImgSize().h);
    }
  }
  auto OutBuffer = req->mOutBufs.find(dstID);
  if (OutBuffer != req->mOutBufs.end()) {
    MY_LOGD("out-img buffer (%#" PRIx64 ") %dx%d stride:%d", OutBuffer->first,
          OutBuffer->second->getImgSize().w, OutBuffer->second->getImgSize().h,
          OutBuffer->second->getBufStridesInBytes(0));

    tmpmcamBufHeaps.push_back(OutBuffer->second);
    auto NSCamBufferHeap = convertImageBufferHeap(OutBuffer->second);
    tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
    auto buf = NSCamBufferHeap->createImageBuffer();
    buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

    // use WROTO
    MCrpRsInfo cropInfo;
    NSCam::NSIoPipe::Output dst;

    cropInfo.mFrameGroup=0;
    cropInfo.mGroupID = WROTO_CROP_GROUP;

    dst.mPortID = PORT_WROTO;
    dst.mPortID.group = 0;
    dst.mBuffer = buf;
    dst.mTransform = trans_sel;
    frameParam.mvOut.push_back(dst);

    //mdp src crop
    cropInfo.mCropRect.s.w = crop[2];
    cropInfo.mCropRect.s.h = crop[3];
    cropInfo.mCropRect.p_integral.x=crop[0];
    cropInfo.mCropRect.p_integral.y=crop[1];
    cropInfo.mCropRect.p_fractional.x=0;
    cropInfo.mCropRect.p_fractional.y=0;
    frameParam.mvCropRsInfo.push_back(cropInfo);
  } else {
    MY_LOGE("Can not find output buffer (id: %d)", dstID);
    return Not_OK;
  }

  auto OutBuffer2 = req->mOutBufs.find(dst2ID);
  if (OutBuffer2 != req->mOutBufs.end()) {
    MY_LOGD("Second out-img buffer (%#" PRIx64 ") %dx%d stride:%d",
      OutBuffer2->first,
      OutBuffer2->second->getImgSize().w, OutBuffer2->second->getImgSize().h,
      OutBuffer2->second->getBufStridesInBytes(0));

    tmpmcamBufHeaps.push_back(OutBuffer2->second);
    auto NSCamBufferHeap = convertImageBufferHeap(OutBuffer2->second);
    tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
    auto buf = NSCamBufferHeap->createImageBuffer();
    buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

    // use WDMAO
    MCrpRsInfo cropInfo;
    NSCam::NSIoPipe::Output dst;

    cropInfo.mFrameGroup=0;
    cropInfo.mGroupID = WDMAO_CROP_GROUP;

    dst.mPortID = PORT_WDMAO;
    dst.mPortID.group = 0;
    dst.mBuffer = buf;
    dst.mTransform = trans_sel;
    frameParam.mvOut.push_back(dst);

    //mdp src crop
    cropInfo.mCropRect.s.w = crop[2];
    cropInfo.mCropRect.s.h = crop[3];
    cropInfo.mCropRect.p_integral.x=crop[0];
    cropInfo.mCropRect.p_integral.y=crop[1];
    cropInfo.mCropRect.p_fractional.x=0;
    cropInfo.mCropRect.p_fractional.y=0;
    frameParam.mvCropRsInfo.push_back(cropInfo);
  } else {
    MY_LOGD_IF(mOpenLog, "No second output buffer (id: %d)", dst2ID);
  }

  // diff.tv_sec = 0;
  // diff.tv_usec = 33 * 1000;
  // gettimeofday(&curr, NULL);
  // timeradd(&curr, &diff, &target);
  // frameParam.ExpectedEndTime.tv_sec = target.tv_sec;
  // frameParam.ExpectedEndTime.tv_usec = target.tv_usec;

  EnqData *eQdata = new EnqData();
  eQdata->requestNo = req->mRequestNo;
  eQdata->mwpDevice = shared_from_this();
  eQdata->mNSCambuf = tmpNSCamBufHeaps;
  eQdata->mmcambuf = tmpmcamBufHeaps;
  // insert WPE QParam
  exParams.CmdIdx = EPIPE_WPE_INFO_CMD;
  exParams.moduleStruct = (void*) &wpeParam;
  frameParam.mvExtraParam.push_back(exParams);

  enQParam.mpfnCallback = WPECallback_Pass;
  enQParam.mpCookie = (void*)eQdata;
  enQParam.mvFrameParams.push_back(frameParam);

  //enque wpe
  MY_LOGD("wpe->enque (%d)", req->mRequestNo);
  auto ret = mpStream->enque(enQParam);

  MY_LOGD_IF(mOpenLog, "convertReqToNormalStream() success");
  return Core_OK;
}

auto
CoreDeviceWPEImpl::
WPECallback_Pass(
    QParams& rParams
) -> void {
  EnqData *pPackage = (EnqData *)(rParams.mpCookie);
  MY_LOGD("WPEImgStreamCallback + , reqNo:%d", pPackage->requestNo);

  if(pPackage == nullptr)
  {
      MY_LOGE("error: rParams.mpCookie is null\n");
      return;
  }

  ResultData result;
  result.requestNo = pPackage->requestNo;
  result.bCompleted = true;

  auto pDevice = pPackage->mwpDevice.lock();
  // release buffer
  MY_LOGD_IF(pDevice->dumpWPE, "unlock buffer");
  int bufNum = 0;
  if (pDevice->dumpImage) {
    for (auto&& buf : pPackage->mmcambuf) {
      auto imgSize    = buf->getImgSize();
      auto imgFormat  = buf->getImgFormat();
      auto imgStride  = buf->getBufStridesInBytes(0);
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
      std::string warpID = std::to_string(pDevice->streamID_warpMap);

      std::string msg = "WPE";
      if (bufNum == 0)
        msg = "WPE_in";
      else if (bufNum == 1)
        msg = "WPE_warp_id" + warpID;
      else if (bufNum == 2)
        msg = "WPE_out";
      else if (bufNum == 3)
        msg = "WPE_out2";

      char szFileName[1024];
      snprintf(szFileName, sizeof(szFileName), "%s%s-PW_%d_%dx%d_%d.%s",
        DUMP_FOLDER, msg.c_str(), pPackage->requestNo,
        imgSize.w, imgSize.h, imgStride, extName.c_str());

      MY_LOGD("dump image requestNo(%d) bufNum(%d) to %s",
        pPackage->requestNo, bufNum, szFileName);
      auto imgBuf = buf->createImageBuffer();
      imgBuf->lockBuf("WPEDevice", mcam::eBUFFER_USAGE_SW_READ_OFTEN);
      imgBuf->saveToFile(szFileName);
      imgBuf->unlockBuf("WPEDevice");

      bufNum++;
    }
  }

  for (auto&& buf : pPackage->mNSCambuf) {
    buf->unlockBuf("coreDeviceWPE");
  }

  if (pPackage->mNSCambuf.size() == 4)
    MY_LOGD_IF(pDevice->dumpWPE, "Erase reqNo(%d) mbuf: %p, %p, %p, %p",
      result.requestNo,
      pPackage->mNSCambuf[0].get(), pPackage->mNSCambuf[1].get(), pPackage->mNSCambuf[2].get(),
      pPackage->mNSCambuf[3].get());
  else
    MY_LOGD_IF(pDevice->dumpWPE, "Erase reqNo(%d) mbuf: %p, %p, %p, tuning: %p",
      result.requestNo,
      pPackage->mNSCambuf[0].get(), pPackage->mNSCambuf[1].get(), pPackage->mNSCambuf[2].get());

  if (pDevice == nullptr) {
    MY_LOGE("pDevice == nullptr, maybe occur buffer leak");
  }
  pPackage->mNSCambuf.erase(pPackage->mNSCambuf.begin(), pPackage->mNSCambuf.end());
  pPackage->mmcambuf.erase(pPackage->mmcambuf.begin(), pPackage->mmcambuf.end());
  // perform callback
  MY_LOGD("performCallback, reqNo: %d", result.requestNo);
  if (pDevice == nullptr || !pDevice->performCallback(result) == Core_OK) {
    MY_LOGE("performCallback Fail, reqNo: %d", result.requestNo);
  }

  if (pDevice->mvReqQueue.empty()) {
    pDevice->mFlushCond.broadcast();
  }

  delete pPackage;

  MY_LOGD("WPEImgStreamCallback - ");
  return;
}

auto
CoreDeviceWPEImpl::
convertReqToDpIspStream(
    shared_ptr<RequestData>& req
) -> int32_t {
  MY_LOGD_IF(mOpenLog, "convertReqToDpIspStream +, req: %d", req->mRequestNo);


  vector<::android::sp<NSCam::IImageBufferHeap>> tmpNSCamBufHeaps;
  vector<shared_ptr<mcam::IImageBufferHeap>> tmpmcamBufHeaps;
  auto reqAppMeta = (req->mpAppControl).get();

  // set input image
  {
    auto InBuffer = req->mInBufs.find(streamID_srcImage);
    if (InBuffer != req->mInBufs.end()) {
      tmpmcamBufHeaps.push_back(InBuffer->second);
      auto NSCamBufferHeap = convertImageBufferHeap(InBuffer->second);
      tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
      auto buf = NSCamBufferHeap->createImageBuffer();
      buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

      void* va[3] = {0};
      MUINT32 mva[3] = {0};
      MUINT32 size[3] = {0};
      DpColorFormat dpFormat;
      if (!toDpColorFormat((NSCam::EImageFormat)(buf->getImgFormat()), dpFormat)) {
          MY_LOGE("unsupport format");
          return -1;
      }
      if (mpDpIspStream->setSrcConfig(
          buf->getImgSize().w,
          buf->getImgSize().h,
          buf->getBufStridesInBytes(0),
          (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0),
          dpFormat,
          DP_PROFILE_FULL_BT601,
          eInterlace_None,
          NULL,
          false) < 0) {
          MY_LOGE("mpDpIspStream->setSrcConfig failed");
          return Not_OK;
      } else {
        MY_LOGD("mpDpIspStream->setSrcConfig success, w/h/stride0/stride1/DPformat = [%d/%d/%d/%d/%d]",
          buf->getImgSize().w, buf->getImgSize().h, buf->getBufStridesInBytes(0),
          (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0), dpFormat);
      }
      for( int i = 0; i < buf->getPlaneCount(); ++i )
      {
          va[i]   = (void*)buf->getBufVA(i);
          mva[i]  = buf->getBufPA(i);
          size[i] = buf->getBufSizeInBytes(i);
      }
      if (mpDpIspStream->queueSrcBuffer(va, mva, size, buf->getPlaneCount()) < 0)
      {
          MY_LOGE("DpIspStream->queueSrcBuffer failed, %d", req->mRequestNo);
          ResultData result;
          result.requestNo = req->mRequestNo;
          result.bCompleted = true;
          performCallback(result);
          return Not_OK;
      } else {
        MY_LOGD("DpIspStream->queueSrcBuffer success, plane0: va/mva/size = [%d/%d/%d]",
                va[0], mva[0], size[0]);
      }

      MY_LOGD_IF(mOpenLog, "srcImage (%#" PRIx64 ") %dx%d stride:%d",
        streamID_srcImage,
        InBuffer->second->getImgSize().w, InBuffer->second->getImgSize().h,
        InBuffer->second->getBufStridesInBytes(0));
    } else {
      MY_LOGE("No input buffer");
    }
  }

  // obtain crop info from vendor tag
  int32_t crop[4] = {0};
  {
    IMetadata::IEntry entry =
                      reqAppMeta->entryFor(MTK_POSTPROCDEV_WPEPQ_CROP_INFO);
    for (int i = 0; i < entry.count(); i++) {
      crop[i] = entry.itemAt(i, Type2Type<int32_t>());
    }
    MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_CROP_INFO, [%d,%d,%d,%d]",
        crop[0], crop[1], crop[2], crop[3]);
    if (entry.count() == 0) {
      MY_LOGD_IF(mOpenLog, "getEntry MTK_POSTPROCDEV_WPEPQ_CROP_INFO Fail");
      crop[2] = req->mInBufs.find(streamID_srcImage)->second->getImgSize().w;
      crop[3] = req->mInBufs.find(streamID_srcImage)->second->getImgSize().h;
      MY_LOGD_IF(mOpenLog, "replace crop info with src image size: (%d.%d)",
              crop[2], crop[3]);
    }
  }

  // obtain flip and rotate info from vendor tag
  uint8_t flip = 0;
  if (IMetadata::getEntry<uint8_t>(reqAppMeta,
      MTK_POSTPROCDEV_WPEPQ_FLIP, flip) != true) {
      MY_LOGD_IF(mOpenLog, "getEntry Fail: MTK_POSTPROCDEV_WPEPQ_FLIP"); }
  MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_FLIP, [%d]", flip);

  uint8_t rotate = 0;
  if (IMetadata::getEntry<uint8_t>(reqAppMeta,
      MTK_POSTPROCDEV_WPEPQ_ROTATE, rotate) != true) {
      MY_LOGD_IF(mOpenLog, "getEntry Fail: MTK_POSTPROCDEV_WPEPQ_ROTATE"); }
  MY_LOGD_IF(mOpenLog, "MTK_POSTPROCDEV_WPEPQ_ROTATE, [%d]", rotate);

  int trans_sel = 0;
  int r = 0, f = 0;
  if (flip == 1) {            // flip vertical
    trans_sel = 2;
    f = 1;
  } else if (flip == 2) {     // flip horizontal
    trans_sel = 1;
    f = 1;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_90) {
    trans_sel = 4;
    r = 90;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_180) {
    trans_sel = 3;
    r = 180;
  } else if (rotate == MTK_POSTPROCDEV_WPEPQ_ROTATE_270) {
    trans_sel = 7;
    r = 270;
  }
  MY_LOGD_IF(mOpenLog, "dst.mTransform: %d", trans_sel);

  // set destination
  int port = 0;
  vector<int32_t> portTable;
  struct timeval diff, curr, target;
  timeval *targetPtr = NULL;
  int64_t dstID = -1, dst2ID = -1;
  MY_LOGD_IF(mOpenLog, "output buffer size: %d", req->mOutBufs.size());
  if (req->mOutBufs.size() > 2) {
    MY_LOGE("Not support more than 2 output buffer");
    return Not_OK;
  } else if (req->mOutBufs.size() == 2 && rotate != 0) {
    MY_LOGE("Not support 2 output buffer with rotate");
    return Not_OK;
  }
  for (auto&& it : req->mOutBufs){
    if (dstID < 0) {
      dstID = it.first;
      MY_LOGD_IF(mOpenLog, "dstID (%d) %dx%d", it.first,
            it.second->getImgSize().w, it.second->getImgSize().h);
    } else {
      dst2ID = it.first;
      MY_LOGD_IF(mOpenLog, "dst2ID (%d) %dx%d", it.first,
            it.second->getImgSize().w, it.second->getImgSize().h);
    }
  }
  auto OutBuffer = req->mOutBufs.find(dstID);
  if (OutBuffer != req->mOutBufs.end()) {
    MY_LOGD("out-img buffer (%#" PRIx64 ") %dx%d stride:%d", OutBuffer->first,
          OutBuffer->second->getImgSize().w, OutBuffer->second->getImgSize().h,
          OutBuffer->second->getBufStridesInBytes(0));

    tmpmcamBufHeaps.push_back(OutBuffer->second);
    auto NSCamBufferHeap = convertImageBufferHeap(OutBuffer->second);
    tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
    auto buf = NSCamBufferHeap->createImageBuffer();
    buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

    void* va[3] = {0};
    MUINT32 mva[3] = {0};
    MUINT32 size[3] = {0};
    DpColorFormat dpFormat;
    DP_PROFILE_ENUM profile;
    for( uint32_t i = 0; i < buf->getPlaneCount(); ++i ) {
        va[i]   = (void*)buf->getBufVA(i);
        mva[i]  = buf->getBufPA(i);
        size[i] = buf->getBufSizeInBytes(i);
    }
    if (!toDpColorFormat((NSCam::EImageFormat)(buf->getImgFormat()), dpFormat)) {
        MY_LOGE("unsupport format");
        return Not_OK;
    }
    // profile = (streamId == mVideoId) ? DP_PROFILE_BT601 : DP_PROFILE_FULL_BT601;
    profile = DP_PROFILE_FULL_BT601;
    if( mpDpIspStream->setRotation(port, r) ) {
        MY_LOGE("mpDpIspStream->setRotation failed");
        return Not_OK;
    }
    if( mpDpIspStream->setFlipStatus(port, f) ) {
        MY_LOGE("mpDpIspStream->setFlipStatus failed");
        return Not_OK;
    }
    if (mpDpIspStream->setDstConfig(
        port,
        buf->getImgSize().w,
        buf->getImgSize().h,
        buf->getBufStridesInBytes(0),
        (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0),
        dpFormat,
        profile,
        eInterlace_None,
        NULL,
        false) < 0) {
        MY_LOGE("mpDpIspStream->setDstConfig failed");
        return Not_OK;
    }
    if( mpDpIspStream->setSrcCrop(port, crop[0], 0, crop[1], 0, crop[2], 0, crop[3], 0) )
    {
        MY_LOGE("mpDpIspStream->setSrcCrop failed");
        return Not_OK;
    }
    if (mpDpIspStream->queueDstBuffer(port, va, mva, size, buf->getPlaneCount()) < 0)
    {
        MY_LOGE("DpIspStream->queueDstBuffer failed");
        return Not_OK;
    }
    MY_LOGD("[DpIspStream Dump] setRotation(port %d, rotation %d), setFlipStatus(port %d, flip %d)"
            "setDstConfig(port %d, w %d, h %d, y_stride %d, uv_stride %d, dpFormat %d, profile %d)"
            "setSrcCrop(port %d, x %d, y %d, w %d, h %d)"
            "queueDstBuffer(port %d, va %d, mva %d, size %d, PlaneCount %d)",
            port, r, port, f,
            port, buf->getImgSize().w, buf->getImgSize().h,
            buf->getBufStridesInBytes(0), (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0),
            dpFormat, profile,
            port, crop[0], crop[1], crop[2], crop[3],
            port, va[0], mva[0], size[0], buf->getPlaneCount());

    portTable.push_back(port);
    port++;
  } else {
    MY_LOGE("Can not find output buffer (id: %d)", dstID);
    return Not_OK;
  }

  auto OutBuffer2 = req->mOutBufs.find(dst2ID);
  if (OutBuffer2 != req->mOutBufs.end()) {
    MY_LOGD("Second out-img buffer (%#" PRIx64 ") %dx%d stride:%d",
      OutBuffer2->first,
      OutBuffer2->second->getImgSize().w, OutBuffer2->second->getImgSize().h,
      OutBuffer2->second->getBufStridesInBytes(0));

    tmpmcamBufHeaps.push_back(OutBuffer2->second);
    auto NSCamBufferHeap = convertImageBufferHeap(OutBuffer2->second);
    tmpNSCamBufHeaps.push_back(NSCamBufferHeap);
    auto buf = NSCamBufferHeap->createImageBuffer();
    buf->lockBuf("coreDeviceWPE", NSCam::eBUFFER_USAGE_SW_READ_OFTEN |
        NSCam::eBUFFER_USAGE_SW_WRITE_OFTEN | NSCam::eBUFFER_USAGE_HW_CAMERA_READWRITE);

    void* va[3] = {0};
    MUINT32 mva[3] = {0};
    MUINT32 size[3] = {0};
    DpColorFormat dpFormat;
    DP_PROFILE_ENUM profile;
    for( uint32_t i = 0; i < buf->getPlaneCount(); ++i ) {
        va[i]   = (void*)buf->getBufVA(i);
        mva[i]  = buf->getBufPA(i);
        size[i] = buf->getBufSizeInBytes(i);
    }
    if (!toDpColorFormat((NSCam::EImageFormat)(buf->getImgFormat()), dpFormat)) {
        MY_LOGE("unsupport format");
        return Not_OK;
    }
    // profile = (streamId == mVideoId) ? DP_PROFILE_BT601 : DP_PROFILE_FULL_BT601;
    profile = DP_PROFILE_FULL_BT601;
    if( mpDpIspStream->setRotation(port, r) ) {
        MY_LOGE("mpDpIspStream->setRotation failed");
        return Not_OK;
    }
    if( mpDpIspStream->setFlipStatus(port, f) ) {
        MY_LOGE("mpDpIspStream->setFlipStatus failed");
        return Not_OK;
    }
    if (mpDpIspStream->setDstConfig(
        port,
        buf->getImgSize().w,
        buf->getImgSize().h,
        buf->getBufStridesInBytes(0),
        (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0),
        dpFormat,
        profile,
        eInterlace_None,
        NULL,
        false) < 0) {
        MY_LOGE("mpDpIspStream->setDstConfig failed");
        return Not_OK;
    }
    if( mpDpIspStream->setSrcCrop(port, crop[0], 0, crop[1], 0, crop[2], 0, crop[3], 0) )
    {
        MY_LOGE("mpDpIspStream->setSrcCrop failed");
        return Not_OK;
    }
    if (mpDpIspStream->queueDstBuffer(port, va, mva, size, buf->getPlaneCount()) < 0)
    {
        MY_LOGE("DpIspStream->queueDstBuffer failed");
        return Not_OK;
    }
    MY_LOGD("[DpIspStream Dump] setRotation(port %d, rotation %d), setFlipStatus(port %d, flip %d)"
            "setDstConfig(port %d, w %d, h %d, y_stride %d, uv_stride %d, dpFormat %d, profile %d)"
            "setSrcCrop(port %d, x %d, y %d, w %d, h %d)"
            "queueDstBuffer(port %d, va %d, mva %d, size %d, PlaneCount %d)",
            port, r, port, f,
            port, buf->getImgSize().w, buf->getImgSize().h,
            buf->getBufStridesInBytes(0), (buf->getPlaneCount() > 1 ? buf->getBufStridesInBytes(1) : 0),
            dpFormat, profile,
            port, crop[0], crop[1], crop[2], crop[3],
            port, va[0], mva[0], size[0], buf->getPlaneCount());

    portTable.push_back(port);
  } else {
    MY_LOGD_IF(mOpenLog, "No second output buffer (id: %d)", dst2ID);
  }

  // start MDP
  {
      diff.tv_sec = 0;
      diff.tv_usec = 33 * 1000;
      gettimeofday(&curr, NULL);
      timeradd(&curr, &diff, &target);
      targetPtr = &target;
  }
  if( mpDpIspStream->startStream(targetPtr) < 0 )
  {
      MY_LOGE("DpIspStream->startStream failed");
      return Not_OK;
  }
  if( mpDpIspStream->dequeueSrcBuffer() < 0 )
  {
      MY_LOGE("DpIspStream->dequeueSrcBuffer failed");
      return Not_OK;
  }
  {
      for ( MUINT32 i = 0; i < portTable.size(); ++i )
      {
          void* va[3] = {0};
          if( mpDpIspStream->dequeueDstBuffer(portTable[i], va) < 0 )
          {
              MY_LOGE("DpIspStream->dequeueDstBuffer[%d] failed", portTable[i]);
              return Not_OK;
          }
      }
  }
  if( mpDpIspStream->stopStream() < 0 )
  {
      MY_LOGE("DpIspStream->stopStream failed");
      return Not_OK;
  }
  if( mpDpIspStream->dequeueFrameEnd() < 0 )
  {
      MY_LOGE("DpIspStream->dequeueFrameEnd failed");
      return Not_OK;
  }

  // callback
  ResultData result;
  result.requestNo = req->mRequestNo;
  result.bCompleted = true;
  // release buffer
  MY_LOGD_IF(dumpWPE, "unlock buffer");
  int bufNum = 0;
  if (dumpImage) {
    for (auto&& buf : tmpmcamBufHeaps) {
      auto imgSize    = buf->getImgSize();
      auto imgFormat  = buf->getImgFormat();
      auto imgStride  = buf->getBufStridesInBytes(0);
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
      // std::string warpID = std::to_string(streamID_warpMap);

      std::string msg = "WPE";
      if (bufNum == 0)
        msg = "MDP_in";
      else if (bufNum == 1)
        msg = "MDP_out";
      else if (bufNum == 2)
        msg = "MDP_out2";

      char szFileName[1024];
      snprintf(szFileName, sizeof(szFileName), "%s%s-PW_%d_%dx%d_%d.%s",
        DUMP_FOLDER, msg.c_str(), req->mRequestNo,
        imgSize.w, imgSize.h, imgStride, extName.c_str());

      MY_LOGD("dump image requestNo(%d) bufNum(%d) to %s",
        req->mRequestNo, bufNum, szFileName);
      auto imgBuf = buf->createImageBuffer();
      imgBuf->lockBuf("WPEDevice", mcam::eBUFFER_USAGE_SW_READ_OFTEN);
      imgBuf->saveToFile(szFileName);
      imgBuf->unlockBuf("WPEDevice");

      bufNum++;
    }
  }
  for (auto&& buf : tmpNSCamBufHeaps) {
    buf->unlockBuf("coreDeviceWPE");
  }

  // perform callback
  MY_LOGD("performCallback, reqNo: %d", result.requestNo);
  if (!performCallback(result) == Core_OK) {
    MY_LOGE("performCallback Fail, reqNo: %d", result.requestNo);
  }

  {
    Mutex::Autolock _l(mReqQueueLock);
    if (mvReqQueue.empty()) {
      mFlushCond.broadcast();
    }
  }

  MY_LOGD_IF(mOpenLog, "convertReqToDpIspStream() success");
  return Core_OK;
}

auto CoreDeviceWPEImpl::closeCore() -> int32_t {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("closeCore + ");

  if (mInited)
  {
    uninit();
  }

  MY_LOGD("closeCore - ");
  return Core_OK;
}

auto CoreDeviceWPEImpl::flushCore() -> int32_t {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("flushCore + ");

  Mutex::Autolock _ll(mReqQueueLock);
  if (!mvReqQueue.empty()) {
    MY_LOGD("wait requests callback");
    mFlushCond.wait(mReqQueueLock);
  }

  MY_LOGD("flushCore - ");
  return Core_OK;
}

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam
