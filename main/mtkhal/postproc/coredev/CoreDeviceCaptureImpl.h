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
#ifndef COREDEV_COREDEVICECAPTUREIMPL_H_
#define COREDEV_COREDEVICECAPTUREIMPL_H_

#include "CoreDeviceBase.h"

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <semaphore.h>
#include <condition_variable>

#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/def/IPostProcDef.h>
#include <mtkcam/utils/exif/IBaseCamExif.h>
#include <mtkcam/utils/exif/StdExif.h>
#include <mtkcam/drv/iopipe/SImager/ISImager.h>

using std::shared_ptr;
using std::vector;
using NSCam::NSIoPipe::NSPostProc::INormalStream;
using NSCam::NSIoPipe::QParams;

namespace NSCam {
namespace v3 {
namespace CoreDev {
namespace model {

class my_encode_params {
 public:
  // buffer
  NSCam::IImageBuffer* pSrc;
  NSCam::IImageBuffer* pDst;
  // settings
  MUINT32 transform;
  MRect crop;
  MUINT32 isSOI;
  MUINT32 quality;
  MUINT32 codecType;
  size_t bsOffset;
  my_encode_params()
  : pSrc(NULL)
  , pDst(NULL)
  , transform(0)
  , crop()
  , isSOI(0)
  , quality(0)
  , codecType(0)
  , bsOffset(0)
  {}
};

class CoreDeviceCaptureImpl
    : public CoreDeviceBase,
      public std::enable_shared_from_this<CoreDeviceCaptureImpl> {
 public:
  virtual ~CoreDeviceCaptureImpl();
  explicit CoreDeviceCaptureImpl(int32_t id);

 public:
  int32_t configureCore(InternalStreamConfig const& config) override;
  int32_t processReqCore(int32_t reqNo,
                                 vector<shared_ptr<RequestData>> reqs) override;
  int32_t closeCore() override;

  int32_t flushCore() override;

  // p2
  auto onProcessFrame(shared_ptr<RequestData> req) -> int32_t;
  // jpeg
  auto onDequeRequest(shared_ptr<RequestData>& req) -> uint32_t;
  auto onProcessJpegFrame(shared_ptr<RequestData> req) -> int32_t;
  uint32_t encodeMainJpeg(shared_ptr<RequestData> req,
    StdExif* pExif);
  void encodeThumbnail(shared_ptr<RequestData> req, StdExif* pExif);

  void startEncode(my_encode_params& rParams);
  void initExif(shared_ptr<RequestData> req, StdExif* pExif);
  void encodeExif(shared_ptr<RequestData> req, StdExif* pExif, uint32_t bsSize);

  static void onP2ACallback(QParams& rParams);
 public:
  bool mStop = false;

 private:
  int32_t mLogLevel = 1;
  int32_t mId = 0;
  int32_t mCamId = -1;
  std::timed_mutex mLock;
  int64_t streamID_ImgIn = -1;
  int64_t streamID_ThumbIn = -1;
  int64_t streamID_WDMAO = -1;
  int64_t streamID_WROTO = -1;
  int64_t streamID_Jpeg = -1;

  // for jpeg
  pthread_t mJpegThread;
  std::vector<shared_ptr<RequestData>> mJpegQueue;
  mutable std::mutex mReqLock;
  std::condition_variable mReqCond;
  std::condition_variable mFlushCond;
  int32_t mUseSWEnc = 0;

  // for yuv
  enum EnqueBufType : int32_t {
    TYPE_IMGI,  // p2 in
    TYPE_WDMAO,
    TYPE_WROTO,
    TYPE_JPEG,
    TYPE_NUM
  };
  struct EnquePackage {
    uint32_t requestNo = 0;
    std::weak_ptr<CoreDeviceCaptureImpl> mwpDevice;
    android::sp<NSCam::IImageBuffer> mbuf[TYPE_NUM];
  };
  INormalStream* mpP2Drv = nullptr;

};

};  // namespace model
};  // namespace CoreDev
};  // namespace v3
};  // namespace NSCam

#endif  // COREDEV_COREDEVICECAPTUREIMPL_H_