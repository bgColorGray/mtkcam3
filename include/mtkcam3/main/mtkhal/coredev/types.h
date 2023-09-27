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
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef INCLUDE_MTKCAM_INTERFACES_COREDEV_TYPES_H_
#define INCLUDE_MTKCAM_INTERFACES_COREDEV_TYPES_H_

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <utils/RefBase.h>

#include "mtkcam3/pipeline/stream/IStreamInfo.h"
#include "mtkcam3/pipeline/stream/StreamId.h"
#include "mtkcam3/main/mtkhal/core/utils/imgbuf/1.x/IImageBuffer.h"
#include "mtkcam/utils/metadata/IMetadata.h"
#include "mtkcam/def/CameraInfo.h"

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace CoreDev {

enum CoreDevType : uint8_t {
  CAPTURE_DEV,
  STREAM_DEV,
  MDP_DEV,
  WPE_DEV,
  JPEG_DEV,
  FD_DEV
};

struct InternalStreamConfig {
  std::vector<int32_t> SensorIdSet;
  std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> vInputInfo;
  std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> vOutputInfo;
  std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>>
      vTuningMetaInfo;
  std::shared_ptr<IMetadata> pConfigureParam = nullptr;
  std::shared_ptr<CameraInfoMapT> pCameraInfoMap = nullptr;
};

//  callback
struct ResultData {
  uint32_t requestNo = 0;
  int32_t status = 0;  // non zero means error
  std::shared_ptr<IMetadata> pMetaRet = nullptr;
  std::shared_ptr<IMetadata> pMetaHal = nullptr;
  bool bCompleted = false;
  std::vector<std::shared_ptr<mcam::IImageBufferHeap>> completedBufs;
};

//  hal3+
struct RegisteredBuf {
  std::shared_ptr<mcam::IImageBufferHeap> pHeap = nullptr;
  void* pHandle = nullptr;
  std::shared_ptr<void> pISPHeap = nullptr;
};

//  mtk hal
struct RequestBuf {
  int64_t streamId;
  uint64_t bufferId = 0;
  std::shared_ptr<mcam::IImageBufferHeap> pHeap = nullptr;
  IMetadata bufData;
};

struct RegisteredData {
  std::vector<RequestBuf> outBufs;
  std::map<int32_t, std::vector<RequestBuf>> inBufs;
  IMetadata appResult;
  IMetadata halResult;
};

};      // namespace CoreDev
};      // namespace v3
};      // namespace NSCam
#endif  // INCLUDE_MTKCAM_INTERFACES_COREDEV_TYPES_H_
