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
#define LOG_TAG "MtkCam/CoreDeviceCapture"

#include "coredev/CoreDeviceCaptureImpl.h"
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include "mtkcam/utils/debug/Properties.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

// capture
#include <dip_reg.h>
#include <mtkcam/drv/def/IPostProcDef.h>
#include <mtkcam3/main/mtkhal/core/utils/imgbuf/ConvertImageBuffer.h>
#include <mtkcam/drv/iopipe/SImager/ISImagerDataTypes.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>



CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_HAL_SERVER);

#define Core_OK 0
#define Not_OK -1

using std::make_shared;
using std::move;
using std::shared_ptr;
using std::vector;

using NSCam::NSIoPipe::QParams;
using NSCam::NSIoPipe::FrameParams;
using NSCam::NSIoPipe::Input;
using NSCam::NSIoPipe::Output;
using NSCam::NSIoPipe::MCropRect;
using NSCam::NSIoPipe::MCrpRsInfo;
using NSCam::NSIoPipe::ExtraParam;
using NSCam::NSIoPipe::NSSImager::ISImager;
using NSCam::NSIoPipe::NSSImager::JPEGENC_SW;
using NSCam::NSIoPipe::NSSImager::JPEGENC_HW_ONLY;
using mcam::CameraBlob;


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

static android::sp<IImageBuffer> gTuningBuffer = nullptr;
static void* gTuningBufAddr = nullptr;

#include "tuningDataGen.h"

static void setCropInfo(
  int32_t width,
  int32_t height,
  int32_t width2,
  int32_t height2,
  int32_t width3,
  int32_t height3,
  MRect cropDMAO,
  MRect cropROTO,
  FrameParams& frameParams) {
  MCrpRsInfo crop;
  crop.mFrameGroup = 0;
  crop.mGroupID = 1;
  MCrpRsInfo crop2;
  crop2.mFrameGroup = 0;
  crop2.mGroupID = 2;
  MCrpRsInfo crop3;
  crop3.mFrameGroup = 0;
  crop3.mGroupID = 3;
  crop.mCropRect.p_fractional.x = 0;
  crop.mCropRect.p_fractional.y = 0;
  crop.mCropRect.p_integral.x = 0;
  crop.mCropRect.p_integral.y = 0;
  crop.mCropRect.s.w = width;
  crop.mCropRect.s.h = height;
  crop.mResizeDst.w = width;
  crop.mResizeDst.h = height;
  crop2.mCropRect.p_fractional.x = 0;
  crop2.mCropRect.p_fractional.y = 0;
  crop2.mCropRect.p_integral.x = cropDMAO.p.x;
  crop2.mCropRect.p_integral.y = cropDMAO.p.y;
  crop2.mCropRect.s.w = cropDMAO.s.w;
  crop2.mCropRect.s.h = cropDMAO.s.h;
  crop2.mResizeDst.w = width2;
  crop2.mResizeDst.h = height2;
  crop3.mCropRect.p_fractional.x = 0;
  crop3.mCropRect.p_fractional.y = 0;
  crop3.mCropRect.p_integral.x = cropROTO.p.x;
  crop3.mCropRect.p_integral.y = cropROTO.p.y;
  crop3.mCropRect.s.w = cropROTO.s.w;
  crop3.mCropRect.s.h = cropROTO.s.h;
  crop3.mResizeDst.w = width3;
  crop3.mResizeDst.h = height3;
  frameParams.mvCropRsInfo.push_back(crop);
  frameParams.mvCropRsInfo.push_back(crop2);
  frameParams.mvCropRsInfo.push_back(crop3);
}

static void SetDefaultTuning(dip_a_reg_t* pIspReg) {
  setCCM_D1_UT(pIspReg);
  setGGM_D1_UT(pIspReg);
  setG2CX_D1_UT(pIspReg);
  setC2G_D1_UT(pIspReg);
  setIGGM_D1_UT(pIspReg);
  setGGM_D3_UT(pIspReg);
  setG2C_D1_UT(pIspReg);
}

template <typename T>
static inline MBOOL
tryGetMetadata(
    IMetadata const* pMetadata,
    MUINT32 const tag,
    T & rVal,
    int32_t index = 0
)
{
    if( pMetadata == NULL ) {
        MY_LOGE("pMetadata == NULL");
        return MFALSE;
    }

    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if( !entry.isEmpty() ) {
        rVal = entry.itemAt(index, Type2Type<T>());
        return MTRUE;
    }
    return MFALSE;
}


static MVOID* jpegThreadLoop(MVOID* arg) {
  CoreDeviceCaptureImpl* impl = reinterpret_cast<CoreDeviceCaptureImpl*>(arg);
  shared_ptr<RequestData> pReq = nullptr;
  uint32_t reqNum = 0;
  while (!impl->mStop || reqNum != 0) {
    reqNum = impl->onDequeRequest(pReq);
    if (pReq != nullptr) {
      impl->onProcessJpegFrame(pReq);
    }
    pReq = nullptr;
  }
  return nullptr;
}

CoreDeviceCaptureImpl::~CoreDeviceCaptureImpl() {
  MY_LOGI("destroy CoreDeviceCaptureImpl");
}

CoreDeviceCaptureImpl::CoreDeviceCaptureImpl(int32_t id) : mId(id) {
  MY_LOGI("create CoreDeviceCaptureImpl");
}

auto CoreDeviceCaptureImpl::configureCore(InternalStreamConfig const& config)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("configure + ");
  mLogLevel = NSCam::Utils::Properties::property_get_int32(
            "vendor.debug.camera.coredevice.capture.log", 0);

  if (config.vInputInfo.size() == 0) {
    MY_LOGW("no input configure");
    return -1;
  }
  if (config.vOutputInfo.size() == 0) {
    MY_LOGW("no output configure");
    return -1;
  }
  streamID_ImgIn = config.vInputInfo.begin()->first;
  if (config.vInputInfo.size() > 1) {
    MSize temp1 = config.vInputInfo.begin()->second->getImgSize();
    auto iter = config.vInputInfo.begin();
    iter++;
    MSize temp2 = iter->second->getImgSize();
    streamID_ThumbIn = iter->first;
    if (temp1.size() < temp2.size()) {
      std::swap(streamID_ImgIn, streamID_ThumbIn);
    }
    MY_LOGD("mainYUV(%lld), thumbYUV(%lld)",
      streamID_ImgIn, streamID_ThumbIn);
  }
  auto mainFmt = config.vInputInfo.at(streamID_ImgIn)->getImgFormat();

  MY_LOGD("mainFmt : 0x%X", mainFmt);
  for (auto& iter: config.vOutputInfo) {
    if ((iter.second->getImgFormat() == eImgFmt_BLOB ||
         iter.second->getImgFormat() == eImgFmt_JPEG) &&
        streamID_Jpeg == -1) {
      streamID_Jpeg = iter.first;
      continue;
    }
    if (streamID_WDMAO == -1) {
      streamID_WDMAO = iter.first;
    } else if (streamID_WROTO == -1) {
      streamID_WROTO = iter.first;
    } else {
      MY_LOGE("not support too many output stream");
      return -1;
    }
  }

  // create tuning buffer
  if (gTuningBuffer == nullptr) {
    IImageBufferAllocator* allocator = IImageBufferAllocator::getInstance();
    IImageBufferAllocator::ImgParam imgParam(sizeof(dip_a_reg_t),0);
    gTuningBuffer = allocator->alloc("PostprocTuningBuf", imgParam);
    if ( !gTuningBuffer->lockBuf( "PostprocTuningBuf",
          (eBUFFER_USAGE_HW_CAMERA_READ | eBUFFER_USAGE_SW_MASK)) )
    {
      MY_LOGE("lock tuning buffer failed\n");
      return -1;
    }
    gTuningBufAddr = (void *)gTuningBuffer->getBufVA(0);
    dip_a_reg_t *pIspReg;
    pIspReg = (dip_a_reg_t *)gTuningBufAddr;
    MY_LOGD("Tuning buffer VA address %p \n", gTuningBufAddr);
    SetDefaultTuning(pIspReg);
  }
  mpP2Drv = INormalStream::createInstance(0xFFFF);

  mStop = false;
  pthread_create(&mJpegThread, NULL, jpegThreadLoop, this);
  pthread_setname_np(mJpegThread, "PostProc@Cap");

  MY_LOGD("in(0x%llX), out1(0x%llX), out2(0x%llX), jpeg(0x%llX)",
    streamID_ImgIn, streamID_WDMAO, streamID_WROTO, streamID_Jpeg);
  MY_LOGD("configure - ");
  return Core_OK;
}

auto CoreDeviceCaptureImpl::processReqCore(int32_t reqNo,
                                          vector<shared_ptr<RequestData>> reqs)
    -> int {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD_IF(mLogLevel, "processReqCore + ");
  int err = Core_OK;
  if (streamID_Jpeg != -1) {
    std::unique_lock<std::mutex> _reqLock(mReqLock);
    for (size_t r = 0; r < reqs.size(); ++r) {
      mJpegQueue.push_back(reqs[r]);
    }
    mReqCond.notify_all();
  } else {
    for (size_t r = 0; r < reqs.size(); ++r) {
      onProcessFrame(reqs[r]);
    }
  }

  MY_LOGD_IF(mLogLevel, "processReqCore - ");
  return err;
}

auto CoreDeviceCaptureImpl::closeCore() -> int32_t {
  std::lock_guard<std::timed_mutex> _l(mLock);
  MY_LOGD("closeCore + ");
  mStop = true;
  flushCore();
  mReqCond.notify_all();
  MY_LOGD("wait thread");
  pthread_join(mJpegThread, NULL);
  MY_LOGD("closeCore - ");
  return Core_OK;
}

auto CoreDeviceCaptureImpl::flushCore() -> int32_t {
  MY_LOGD("flushCore + ");
  std::unique_lock<std::mutex> _reqLock(mReqLock);
  /*
  if (!mReqList.empty()) {
    MY_LOGD("wait requests callback");
    mFlushCond.wait(_reqLock);
  }
  */
  MY_LOGD("flushCore - ");
  return Core_OK;
}

auto CoreDeviceCaptureImpl::onDequeRequest(shared_ptr<RequestData>& req)
    -> uint32_t {
  std::unique_lock<std::mutex> _l(mReqLock);
  MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_0");
  while (mJpegQueue.empty() && !mStop) {
    mReqCond.wait(_l);
  }
  if (!mJpegQueue.empty()) {
    MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_1");
    req = *mJpegQueue.begin();
    mJpegQueue.erase(mJpegQueue.begin());
  } else {
    MY_LOGD_IF(mLogLevel, "[onDequeRequest] In_2");
    mFlushCond.notify_all();
  }
  return mJpegQueue.size();
}

#define MAX_THUMBNAIL_SIZE ((160) * (120) * 18 / 10)

void
CoreDeviceCaptureImpl::
startEncode(
  my_encode_params& rParams
)
{
  MY_LOGD("+ src: %dx%d, dst: %dx%d, format: 0x%x, crop(%d,%d;%d,%d), bsOffset(%zu)",
            rParams.pSrc-> getImgSize().w, rParams.pSrc-> getImgSize().h,
            rParams.pDst-> getImgSize().w, rParams.pDst-> getImgSize().h,
            rParams.pDst->getImgFormat(),
            rParams.crop.p.x, rParams.crop.p.y,
            rParams.crop.s.w, rParams.crop.s.h,
            rParams.bsOffset
           );

    //
  MBOOL ret = MTRUE;
    //
  ISImager* pSImager = ISImager::createInstance(rParams.pSrc);
  if( pSImager == NULL ) {
    MY_LOGE("create SImage failed");
    return;
  }

  ret = pSImager->setTargetImgBuffer(rParams.pDst)
        && pSImager->setTransform(rParams.transform)
        && pSImager->setCropROI(rParams.crop)
        && pSImager->setEncodeParam(
                rParams.isSOI,
                rParams.quality,
                rParams.codecType,
                false,
                rParams.bsOffset
                )
        && pSImager->execute();

  pSImager->destroyInstance();
  pSImager = NULL;
  //
  if (!ret) {
    MY_LOGE("encode failed");
    return;
  }
  //
  MY_LOGD("- bistream size %zu. Check bit: 0x%x 0x%x", rParams.pDst->getBitstreamSize(),
          (*(reinterpret_cast<MUINT8*>(rParams.pDst->getBufVA(0)))),
          *(reinterpret_cast<MUINT8*>(rParams.pDst->getBufVA(0))+1));
}


uint32_t
CoreDeviceCaptureImpl::
encodeMainJpeg(shared_ptr<RequestData> req, StdExif* pExif) {
  my_encode_params params;
  std::shared_ptr<mcam::IImageBufferHeap> inBuf = nullptr;
  std::shared_ptr<mcam::IImageBufferHeap> OutBuf = nullptr;
  android::sp<NSCam::IImageBuffer> psrcBuf = nullptr;
  android::sp<NSCam::IImageBuffer> pdstBuf = nullptr;

  if (req->mInBufs.count(streamID_ImgIn) > 0) {
    inBuf = req->mInBufs.at(streamID_ImgIn);
  } else {
    MY_LOGE("WithOut input main buffer");
    return 0;
  }
  if (req->mOutBufs.count(streamID_Jpeg) > 0) {
    OutBuf = req->mOutBufs.at(streamID_Jpeg);
  } else {
    MY_LOGE("WithOut output jpeg buffer");
    return 0;
  }

  psrcBuf = mcam::convertImageBufferHeap(inBuf)->createImageBuffer();
  psrcBuf->lockBuf("CoreDevCap-JPGIN",
    eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);

  auto mainFmt = psrcBuf->getImgFormat();
  bool useSWEnc = true;
  if (mainFmt == eImgFmt_YUY2 ||
      mainFmt == eImgFmt_NV12 ||
      mainFmt == eImgFmt_NV21) {
    useSWEnc = false;
  } else {
    useSWEnc = true;
  }

  size_t bufStridesInBytes[3] = {0};
  MSize jpegSize = inBuf->getImgSize();
  int32_t bsOffset = (pExif == nullptr) ? 0 : pExif->getHeaderSize();
  bufStridesInBytes[0] = OutBuf->getBufStridesInBytes(0) - bsOffset;
  pdstBuf = mcam::convertImageBufferHeap(OutBuf)->createImageBuffer_FromBlobHeap(
    bsOffset, eImgFmt_JPEG, jpegSize, bufStridesInBytes);

  pdstBuf->lockBuf("CoreDevCap-JPGOUT", eBUFFER_USAGE_HW_CAMERA_READWRITE
    | eBUFFER_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

  MY_LOGD("out image buffer format : 0x%X", pdstBuf->getImgFormat());
  MY_LOGD("out image buffer VA : 0x%llX", pdstBuf->getBufVA(0));

  params.pSrc = psrcBuf.get();
  params.pDst = pdstBuf.get();
  params.transform = 0;
  params.crop = MRect(MPoint(0,0), inBuf->getImgSize());
  params.isSOI = (pExif != nullptr) ? 0 : 1;
  params.quality = 100;
  params.bsOffset = bsOffset;
  params.codecType = useSWEnc ? JPEGENC_SW : JPEGENC_HW_ONLY;
  MY_LOGD("useSWEnc : %d", useSWEnc);
  //
  startEncode(params);
  uint32_t bsSize = pdstBuf->getBitstreamSize();
  psrcBuf->unlockBuf("CoreDevCap-JPGIN");
  pdstBuf->unlockBuf("CoreDevCap-JPGOUT");
  return bsSize;

}

void
CoreDeviceCaptureImpl::
encodeThumbnail(
  shared_ptr<RequestData> req,
  StdExif* pExif
  ) {
  my_encode_params params;
  std::shared_ptr<mcam::IImageBufferHeap> inBuf = nullptr;
  std::shared_ptr<mcam::IImageBufferHeap> OutBuf = nullptr;
  android::sp<NSCam::IImageBuffer> psrcBuf = nullptr;
  android::sp<NSCam::IImageBuffer> pdstBuf = nullptr;

  if (req->mOutBufs.count(streamID_Jpeg) > 0) {
    OutBuf = req->mOutBufs.at(streamID_Jpeg);
  } else {
    MY_LOGE("WithOut output jpeg buffer");
    return ;
  }
  //
  if (req->mInBufs.count(streamID_ThumbIn) > 0) {
    inBuf = req->mInBufs.at(streamID_ThumbIn);
  } else {
    MY_LOGE("WithOut input thumbnail buffer");
    return ;
  }

  psrcBuf = mcam::convertImageBufferHeap(inBuf)->createImageBuffer();
  psrcBuf->lockBuf("CoreDevCap-ThumbIN", eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);

  size_t bufStridesInBytes[3] = {0};
  if (MAX_THUMBNAIL_SIZE < (OutBuf->getBufStridesInBytes(0) - pExif->getStdExifSize())) {
    bufStridesInBytes[0] = MAX_THUMBNAIL_SIZE;
  } else {
    bufStridesInBytes[0] = OutBuf->getBufStridesInBytes(0) - pExif->getStdExifSize();
  }
  MSize jpegSize = inBuf->getImgSize();
  pdstBuf = mcam::convertImageBufferHeap(OutBuf)->createImageBuffer_FromBlobHeap(
    pExif->getStdExifSize(), eImgFmt_JPEG, jpegSize, bufStridesInBytes);

  pdstBuf->lockBuf("CoreDevCap-ThumbOut", eBUFFER_USAGE_HW_CAMERA_READWRITE
    | eBUFFER_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

  MY_LOGD("out image buffer format : 0x%X", pdstBuf->getImgFormat());

  params.pSrc = psrcBuf.get();
  params.pDst = pdstBuf.get();
  params.transform = 0;
  params.crop = MRect(MPoint(0,0), inBuf->getImgSize());
  params.isSOI = 1;
  params.quality = 100;
  params.bsOffset = (pExif == nullptr) ? 0 : pExif->getStdExifSize();
  params.codecType = JPEGENC_SW;
  //
  startEncode(params);
  psrcBuf->unlockBuf("CoreDevCap-ThumbIN");
  pdstBuf->unlockBuf("CoreDevCap-ThumbOut");
  return ;

}


void
CoreDeviceCaptureImpl::
initExif(shared_ptr<RequestData> req, StdExif* pExif) {
  std::shared_ptr<mcam::IImageBufferHeap> inBuf = nullptr;
  if (req->mInBufs.count(streamID_ImgIn) > 0) {
    inBuf = req->mInBufs.at(streamID_ImgIn);
  } else {
    MY_LOGE("WithOut input main buffer");
    return;
  }
  ExifParams params;
  params.u4ImageWidth  = inBuf->getImgSize().w;
  params.u4ImageHeight = inBuf->getImgSize().h;
  IMetadata* appMeta = req->mpAppControl.get();
  MY_LOGD("no tag: MTK_3A_EXIF_METADATA, miss in Hal meta find APP meta");
  MFLOAT fNumber = 0.0f;
  if (tryGetMetadata<MFLOAT>(appMeta, MTK_LENS_APERTURE, fNumber)) {
    params.u4FNumber = fNumber*FNUMBER_PRECISION;
  }
  MFLOAT focalLength = 0.0f;
  if (tryGetMetadata<MFLOAT>(appMeta, MTK_LENS_FOCAL_LENGTH, focalLength)) {
    params.u4FocalLength = focalLength*1000;
  }
  MINT64 capExposure = 0;
  if (tryGetMetadata<MINT64>(appMeta, MTK_SENSOR_EXPOSURE_TIME, capExposure)) {
    params.u4CapExposureTime = (MINT32)(capExposure/1000);
  }
  MINT32 temp = 0;
  tryGetMetadata<MINT32>(appMeta, MTK_SENSOR_SENSITIVITY, temp);
  params.u4AEISOSpeed = temp;

  params.u4Orientation = 0;
  MFLOAT zoomRatio = 1.0f;
  tryGetMetadata<MFLOAT>(appMeta, MTK_CONTROL_ZOOM_RATIO, zoomRatio);
  params.u4ZoomRatio = (MUINT32) (zoomRatio * 100);
  params.u4Facing = 0;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  params.seconds = tv.tv_sec;
  params.microSeconds = tv.tv_usec;
  params.bNeedSOI = MTRUE;

  pExif->init(params, 0);

  //set thumbnail max size & thumbnail size need to be 128 alignment
  size_t thumbMaxSize = MAX_THUMBNAIL_SIZE;

  //if (1)
  {
#define EXIFAPP1_MAX_SIZE 65535 // 64K exif appn max data size
#define EXIFHEADER_ALIGN 128

    size_t thumbnailSize = 0;
    if ((EXIFAPP1_MAX_SIZE - pExif->getStdExifSize()) < thumbMaxSize) {
      thumbnailSize = EXIFAPP1_MAX_SIZE - pExif->getStdExifSize();
      size_t res = thumbnailSize % EXIFHEADER_ALIGN;
      if( res != 0 )
        thumbnailSize = thumbnailSize - res;
    } else {
      thumbnailSize = thumbMaxSize;
      size_t res = thumbnailSize % EXIFHEADER_ALIGN;
      if (res != 0) {
        // prevent it would exceed EXIFAPP1_MAX_SIZE after doing thumbnail size 128 alignemt
        if(thumbnailSize + EXIFHEADER_ALIGN > EXIFAPP1_MAX_SIZE) {
          thumbnailSize -= res;
        } else {
          thumbnailSize = thumbnailSize + EXIFHEADER_ALIGN -res;
        }
      }
    }
    thumbMaxSize = thumbnailSize;
  }

  size_t headerSize = pExif->getStdExifSize() +
        pExif->getDbgExifSize() + thumbMaxSize;
  if (headerSize % EXIFHEADER_ALIGN != 0)
    MY_LOGW("not aligned header size %zu", headerSize);

  pExif->setMaxThumbnail(thumbMaxSize);

}

void
CoreDeviceCaptureImpl::
encodeExif(
  shared_ptr<RequestData> req,
  StdExif* pExif,
  uint32_t bsSize) {

  std::shared_ptr<mcam::IImageBufferHeap> OutBuf = nullptr;
  android::sp<NSCam::IImageBuffer> pdstBuf = nullptr;

  if (req->mOutBufs.count(streamID_Jpeg) > 0) {
    OutBuf = req->mOutBufs.at(streamID_Jpeg);
  } else {
    MY_LOGE("WithOut output jpeg buffer");
    return ;
  }

  pdstBuf = mcam::convertImageBufferHeap(OutBuf)->createImageBuffer();
  pdstBuf->lockBuf("CoreDevCap-ExifOut",
                   eBUFFER_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

  size_t exifSize  = 0;
  MUINTPTR pExifBuf = pdstBuf->getBufVA(0);
  MY_LOGD("out pExifBuf VA : 0x%llX", pExifBuf);
  if (pExifBuf == 0) {
    MY_LOGE("get exif buffer address failed");
    return;
  }
  int32_t ret = pExif->make(pExifBuf, exifSize, true);
  pExif->uninit();
  if (ret) {
    MY_LOGE("encode exif error");
    return;
  }
  size_t JpegBitStreamSize = 0;
  JpegBitStreamSize = pExif->getHeaderSize() + bsSize;
  pdstBuf->setBitstreamSize(JpegBitStreamSize);
  OutBuf->setBitstreamSize(JpegBitStreamSize);
  size_t bufSize = pdstBuf->getBufSizeInBytes(0);
  MY_LOGD("buffer size : %d", bufSize);
  CameraBlob* pTransport = reinterpret_cast<CameraBlob*>(pExifBuf + bufSize - sizeof(CameraBlob));
  pTransport->blobId = mcam::CameraBlobId::JPEG;
  pTransport->blobSize = JpegBitStreamSize;
  MY_LOGD("CameraBlob added: jpegBuf:%#" PRIxPTR " bufsize:%zu datasize:%zu",
    pExifBuf, bufSize, JpegBitStreamSize);

  pdstBuf->unlockBuf("CoreDevCap-ExifOut");

}

auto CoreDeviceCaptureImpl::onProcessJpegFrame(shared_ptr<RequestData> req)
    -> int32_t {

  // currently, thumbnailsize use default 160x120.
  StdExif exif;
  initExif(req, &exif);
  uint32_t bsSize = encodeMainJpeg(req, &exif);
  encodeThumbnail(req, &exif);
  encodeExif(req, &exif, bsSize);
  ResultData result;
  result.requestNo = req->mRequestNo;
  result.bCompleted = true;
  performCallback(result);
  return 0;
}



auto CoreDeviceCaptureImpl::onProcessFrame(shared_ptr<RequestData> req)
    -> int32_t {
  std::shared_ptr<mcam::IImageBufferHeap> inBuf = nullptr;
  std::shared_ptr<mcam::IImageBufferHeap> DMAOBuf = nullptr;
  std::shared_ptr<mcam::IImageBufferHeap> ROTOBuf = nullptr;
  int ret = 0;
  QParams enqueParams;
  FrameParams frameParams;
  Input src;

  if (req->mInBufs.count(streamID_ImgIn) > 0) {
    inBuf = req->mInBufs.at(streamID_ImgIn);
  } else {
    MY_LOGE("WithOut input buffer");
    return -1;
  }

  if (req->mOutBufs.count(streamID_WDMAO) > 0) {
    DMAOBuf = req->mOutBufs.at(streamID_WDMAO);
  }
  if (req->mOutBufs.count(streamID_WROTO) > 0) {
    ROTOBuf = req->mOutBufs.at(streamID_WROTO);
  }
  frameParams.mStreamTag = NSCam::NSIoPipe::NSPostProc::ENormalStreamTag_Normal;
  MUINT32 imgiW, imgiH, wdmaoW, wdmaoH, wrotoW, wrotoH;
  MRect cropDMAO, cropROTO;
  android::sp<NSCam::IImageBuffer> srcBuffer;
  android::sp<NSCam::IImageBuffer> dstDMAOBuf = nullptr;
  android::sp<NSCam::IImageBuffer> dstROTOBuf = nullptr;
  IMetadata* appMeta = req->mpAppControl.get();
  imgiW = inBuf->getImgSize().w;
  imgiH = inBuf->getImgSize().h;
  if (DMAOBuf != nullptr) {
    wdmaoW = DMAOBuf->getImgSize().w;
    wdmaoH = DMAOBuf->getImgSize().h;
  } else {
    wdmaoW = imgiW;
    wdmaoH = imgiH;
  }
  if (ROTOBuf != nullptr) {
    wrotoW = ROTOBuf->getImgSize().w;
    wrotoH = ROTOBuf->getImgSize().h;
  } else {
    wrotoW = imgiW;
    wrotoH = imgiH;
  }
  // set image buffer
  srcBuffer = mcam::convertImageBufferHeap(inBuf)->createImageBuffer();
  srcBuffer->lockBuf("CoreDevCap-IN", eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);
  src.mPortID = NSIoPipe::PORT_IMGI;
  src.mBuffer = srcBuffer.get();
  src.mPortID.group = 0;
  frameParams.mvIn.push_back(src);
  MINT32 temp = 0;
  if (tryGetMetadata<MINT32>(appMeta, MTK_POSTPROCDEV_CAPTURE_CROP_REGION, temp, 0)) {
    cropDMAO.p.x = temp;
    tryGetMetadata<MINT32>(appMeta, MTK_POSTPROCDEV_CAPTURE_CROP_REGION, temp, 1);
    cropDMAO.p.y = temp;
    tryGetMetadata<MINT32>(appMeta, MTK_POSTPROCDEV_CAPTURE_CROP_REGION, temp, 2);
    cropDMAO.s.w = temp;
    tryGetMetadata<MINT32>(appMeta, MTK_POSTPROCDEV_CAPTURE_CROP_REGION, temp, 3);
    cropDMAO.s.h = temp;
    cropROTO = cropDMAO;
  } else {
    cropROTO.p.x = cropDMAO.p.x = 0;
    cropROTO.p.y = cropDMAO.p.y = 0;
    cropROTO.s.w = cropDMAO.s.w = imgiW;
    cropROTO.s.h = cropDMAO.s.h = imgiH;
  }
  MY_LOGD("crop : (%d, %d) (%dx%d)", cropROTO.p.x, cropROTO.p.y, cropROTO.s.w, cropROTO.s.h);
  setCropInfo(imgiW, imgiH,
              wdmaoW, wdmaoH,
              wrotoW, wrotoH,
              cropDMAO, cropROTO, frameParams);
  if (DMAOBuf != nullptr) {
    Output dst;
    dstDMAOBuf = mcam::convertImageBufferHeap(DMAOBuf)->createImageBuffer();
    dstDMAOBuf->lockBuf("CoreDevCap-DMAO",
      eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);
    dst.mPortID = NSIoPipe::PORT_WDMAO;
    dst.mBuffer = dstDMAOBuf.get();
    dst.mPortID.group = 0;
    frameParams.mvOut.push_back(dst);
  }
  if (ROTOBuf != nullptr) {
    Output dst;
    dstROTOBuf = mcam::convertImageBufferHeap(ROTOBuf)->createImageBuffer();
    dstROTOBuf->lockBuf("CoreDevCap-ROTO",
      eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);
    dst.mPortID = NSIoPipe::PORT_WROTO;
    dst.mBuffer = dstROTOBuf.get();
    dst.mPortID.group = 0;
    frameParams.mvOut.push_back(dst);
  }
  // enque
  EnquePackage* pPackage = new EnquePackage();
  pPackage->requestNo = req->mRequestNo;
  pPackage->mwpDevice = shared_from_this();
  pPackage->mbuf[TYPE_IMGI] = srcBuffer;
  pPackage->mbuf[TYPE_WDMAO] = dstDMAOBuf;
  pPackage->mbuf[TYPE_WROTO] = dstROTOBuf;
  frameParams.mTuningData = gTuningBufAddr;
  enqueParams.mpfnCallback = onP2ACallback;
  enqueParams.mpCookie = reinterpret_cast<void*>(pPackage);
  enqueParams.mvFrameParams.push_back(frameParams);
  #if 0
  {
    std::unique_lock<std::mutex> _l(mReqLock);
    mReqList.push_back(req);
  }
  #endif
  ret = mpP2Drv->enque(enqueParams);

  return 0;
}

auto
CoreDeviceCaptureImpl::
onP2ACallback(QParams& rParams) -> void {
  ResultData result;
  EnquePackage* pPackage = reinterpret_cast<EnquePackage*>(rParams.mpCookie);
  result.requestNo = pPackage->requestNo;
  result.bCompleted = true;
  if (pPackage->mbuf[TYPE_IMGI] != nullptr) {
    pPackage->mbuf[TYPE_IMGI]->unlockBuf("CoreDevCap-IN");
    pPackage->mbuf[TYPE_IMGI] = nullptr;
  }
  if (pPackage->mbuf[TYPE_WDMAO] != nullptr) {
    pPackage->mbuf[TYPE_WDMAO]->unlockBuf("CoreDevCap-DMAO");
    pPackage->mbuf[TYPE_WDMAO] = nullptr;
  }
  if (pPackage->mbuf[TYPE_WROTO] != nullptr) {
    pPackage->mbuf[TYPE_WROTO]->unlockBuf("CoreDevCap-ROTO");
    pPackage->mbuf[TYPE_WROTO] = nullptr;
  }
  auto pDevice = pPackage->mwpDevice.lock();
  if (pDevice == nullptr) {
    MY_LOGE("device already be destroyed");
    return;
  }
  #if 0
  {
    std::unique_lock<std::mutex> _l(pDevice->mReqLock);
    for (auto& req = pDevice->mReqList.begin();
           req != pDevice->mReqList.end();
           req++) {
      if (req->mRequestNo == pPackage->requestNo) {
        pDevice->mReqList.erase(req);
        break;
      }
    }
  }
  #endif
  if (!pDevice->performCallback(result) == Core_OK) {
    MY_LOGE("performCallback Fail, reqNo: %d", result.requestNo);
  }
  delete pPackage;
}

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam
