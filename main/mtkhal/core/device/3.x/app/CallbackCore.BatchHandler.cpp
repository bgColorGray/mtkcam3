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

#include "CallbackCore.h"
#include "MyUtils.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

using ::android::Mutex;
using ::android::OK;
using ::android::Vector;

#define ThisNamespace CallbackCore::BatchHandler

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
#define MY_DEBUG(level, fmt, arg...)                                           \
  do {                                                                         \
    CAM_ULOGM##level("[%s][%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                                   \
  } while (0)

#define MY_WARN(level, fmt, arg...)                                        \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR_ULOG(level, fmt, arg...)                                  \
  do {                                                                     \
    CAM_ULOGM##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                     ##arg);                                               \
  } while (0)

#define MY_ERROR(level, fmt, arg...)                                     \
  do {                                                                   \
    CAM_LOG##level("[%s::%s] " fmt, mInstanceName.c_str(), __FUNCTION__, \
                   ##arg);                                               \
  } while (0)

#define MY_LOGV(...) MY_DEBUG(V, __VA_ARGS__)
#define MY_LOGD(...) MY_DEBUG(D, __VA_ARGS__)
#define MY_LOGI(...) MY_DEBUG(I, __VA_ARGS__)
#define MY_LOGW(...) MY_WARN(W, __VA_ARGS__)
#define MY_LOGE(...) MY_ERROR_ULOG(E, __VA_ARGS__)
#define MY_LOGA(...) MY_ERROR(A, __VA_ARGS__)
#define MY_LOGF(...) MY_ERROR(F, __VA_ARGS__)
//
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

namespace mcam {
namespace core {
/******************************************************************************
 *
 ******************************************************************************/
ThisNamespace::BatchHandler(std::shared_ptr<CommonInfo> pCommonInfo)
    : mInstanceName{pCommonInfo->userName + "-" +
                    std::to_string(pCommonInfo->mInstanceId) + "-BatchHandler"},
      mCommonInfo(pCommonInfo),
      mBatchCounter(0) {}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::resetBatchStreamId() -> void {
  Mutex::Autolock _l(mLock);
  mBatchedStreams.clear();
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::checkStreamUsageforBatchMode(
    const std::shared_ptr<StreamInfo> pStreamInfo) -> bool {
  // check if the stream usage include video_ENCODER
  if ((pStreamInfo->streamType == StreamType::OUTPUT) &&
      (pStreamInfo->consumerUsage & GRALLOC_USAGE_HW_VIDEO_ENCODER)) {
    mBatchedStreams.push_back(pStreamInfo->getStreamId());
    MY_LOGD("Stream %#" PRIx64 " is registered to Batch Stream",
            pStreamInfo->getStreamId());
    return true;
  } else {
    return false;
  }
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::registerBatch(const std::vector<CaptureRequest> requests)
    -> int {
  Mutex::Autolock _l(mLock);
  auto batch = std::make_shared<InFlightBatch>();
  batch->mBatchNo = mBatchCounter;
  batch->mFirstFrame = requests[0].frameNumber;
  batch->mBatchSize = requests.size();
  batch->mLastFrame = batch->mFirstFrame + batch->mBatchSize - 1;
  batch->mHasLastPartial.clear();
  batch->mShutterReturned.clear();
  batch->mFrameHasMetaResult = 0;
  batch->mFrameHasImageResult = 0;
  batch->mFrameRemovedCount = 0;
  mInFlightBatches.push_back(batch);
  MY_LOGD("Batch %d is registered. frameNumber: %d - %d", batch->mBatchNo,
          batch->mFirstFrame, batch->mLastFrame);

  mBatchCounter++;

  return batch->mBatchNo;
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::removeBatchLocked(uint32_t batchNo) -> void {
  Mutex::Autolock _l(mLock);

  auto it = mInFlightBatches.begin();
  while (it != mInFlightBatches.end()) {
    if ((*it)->mBatchNo == batchNo) {
      mInFlightBatches.erase(it);
      MY_LOGD("Batch %d is removed.", batchNo);
      return;
    }
    it++;
  }
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::getBatchLocked(uint32_t frameNumber)
    -> std::shared_ptr<CallbackCore::BatchHandler::InFlightBatch> {
  Mutex::Autolock _l(mLock);
  int numBatches = mInFlightBatches.size();
  for (int i = 0; i < numBatches; i++) {
    if (frameNumber >= mInFlightBatches[i]->mFirstFrame &&
        frameNumber <= mInFlightBatches[i]->mLastFrame) {
      return mInFlightBatches[i];
    }
  }
  return NULL;
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::destroy() -> void {}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpState(IPrinter& printer,
                              const std::vector<std::string> &
                              /*options*/) -> void {
  Mutex::Autolock _lcb(mMergedParcelLock);
  dumpStateLocked(printer);
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::dumpStateLocked(IPrinter& printer) const -> void {
  for (CallbackParcel cb : mMergedParcels) {
    ::android::String8 str;
    str += ::android::String8::format(
        "CallbackParcel: frameNumber(%u) valid(%d)", cb.frameNumber, cb.valid);
    str += "vInputImage: ";
    for (size_t i = 0; i < cb.vInputImageItem.size(); i++) {
      str += ::android::String8::format("(%zu:%p) ", i,
                                        cb.vInputImageItem[i].buffer.get());
    }

    str += "; vOutputImage: ";
    for (size_t i = 0; i < cb.vOutputImageItem.size(); i++) {
      str += ::android::String8::format("(%zu:%p) ", i,
                                        cb.vOutputImageItem[i].buffer.get());
    }
    str += "; vOutputMeta: ";
    for (size_t i = 0; i < cb.vOutputMetaItem.size(); i++) {
      str += ::android::String8::format("(%zu:%p:%u) ", i,
                                        &cb.vOutputMetaItem[i].metadata,
                                        cb.vOutputMetaItem[i].bufferNo);
    }
    str += "; vError: ";
    for (size_t i = 0; i < cb.vError.size(); i++) {
      str += ::android::String8::format(
          "(%zu:%p:%d) ", i, cb.vError[i].stream.get(), cb.vError[i].errorCode);
    }
    str += "; Shutter: ";
    if (cb.shutter.get())
      str += ::android::String8::format("(%p:%" PRIu64 ") ", cb.shutter.get(),
                                        cb.shutter->timestamp);
    printer.printLine(str.string());
  }
}
/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::waitUntilDrained(nsecs_t const timeout __unused) -> int {
  return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::push(std::list<CallbackParcel>& item) -> void {
  if (mBatchedStreams.size() > 0) {
    std::list<CallbackParcel> updatedList;
    updateCallback(item, updatedList);
    mCommonInfo->mpfnCallback(updatedList, mCommonInfo->mpUser);
  } else {
    mCommonInfo->mpfnCallback(item, mCommonInfo->mpUser);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::appendParcel(CallbackParcel srcParcel,
                                 CallbackParcel& dstParcel) -> void {
  if (srcParcel.shutter != nullptr && srcParcel.shutter->timestamp != 0) {
    if (dstParcel.shutter != nullptr) {
      MY_LOGW("Redundantly update shutter timestamp for frameNumber %u",
              srcParcel.frameNumber);
    }
    dstParcel.shutter = srcParcel.shutter;
    // dstParcel.shutter->timestamp = srcParcel.shutter->timestamp;
  }
  if (srcParcel.vOutputMetaItem.size() > 0) {
    Vector<CallbackParcel::MetaItem>* pvMetaCbItem = &dstParcel.vOutputMetaItem;
    if (pvMetaCbItem == nullptr) {
      MY_LOGE("null Dst output pvMetaCbItem");
      return;
    }
    for (CallbackParcel::MetaItem MI : srcParcel.vOutputMetaItem) {
      CallbackParcel::MetaItem& rCbItem =
          pvMetaCbItem->editItemAt(pvMetaCbItem->add());
      rCbItem.metadata = MI.metadata;
      rCbItem.bufferNo = MI.bufferNo;
    }
  }
  if (srcParcel.vOutputImageItem.size() > 0) {
    Vector<CallbackParcel::ImageItem>* pvImageCbItem =
        &dstParcel.vOutputImageItem;
    if (pvImageCbItem == nullptr) {
      MY_LOGE("null Dst output pvImageCbItem");
      return;
    }
    for (CallbackParcel::ImageItem II : srcParcel.vOutputImageItem) {
      CallbackParcel::ImageItem& rCbItem =
          pvImageCbItem->editItemAt(pvImageCbItem->add());
      rCbItem.buffer = II.buffer;
      rCbItem.stream = II.stream;
    }
  }
  if (srcParcel.vInputImageItem.size() > 0) {
    Vector<CallbackParcel::ImageItem>* pvImageCbItem =
        &dstParcel.vInputImageItem;
    if (pvImageCbItem == nullptr) {
      MY_LOGE("null Dst input pvImageCbItem");
      return;
    }
    for (CallbackParcel::ImageItem II : srcParcel.vInputImageItem) {
      CallbackParcel::ImageItem& rCbItem =
          pvImageCbItem->editItemAt(pvImageCbItem->add());
      rCbItem.buffer = II.buffer;
      rCbItem.stream = II.stream;
    }
  }
  if (srcParcel.vError.size() > 0) {
    Vector<CallbackParcel::Error>* pvError = &dstParcel.vError;
    if (pvError == nullptr) {
      MY_LOGE("null Dst pvError");
      return;
    }
    for (CallbackParcel::Error EI : srcParcel.vError) {
      CallbackParcel::Error& rCbItem = pvError->editItemAt(pvError->add());
      rCbItem.stream = EI.stream;
      rCbItem.errorCode = EI.errorCode;
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
auto ThisNamespace::updateCallback(std::list<CallbackParcel> cbParcels,
                                   std::list<CallbackParcel>& rUpdatedList)
    -> void {
  Mutex::Autolock _lcb(mMergedParcelLock);

  bool isPending;
  bool hasPendingMetaResult;
  ::android::Vector<CallbackParcel> mPendingNonBatchParcels;

#define ELEMENT_FOUND(l, v) (std::find(l.begin(), l.end(), v) != l.end())

  for (CallbackParcel cbParcel : cbParcels) {
    auto batch = getBatchLocked(cbParcel.frameNumber);
    // bypass result for non-batch frame
    if (!batch) {
      if (mMergedParcels.size() == 0) {
        rUpdatedList.push_back(cbParcel);
      } else {
        mPendingNonBatchParcels.push(cbParcel);
      }
      continue;
    }

    if (cbParcel.needRemove)
      batch->mFrameRemoved.push_back(cbParcel.frameNumber);

    isPending = false;
    hasPendingMetaResult = false;
    for (CallbackParcel& cbPending : mMergedParcels) {
      if (cbParcel.frameNumber != cbPending.frameNumber)
        continue;

      isPending = true;
      // copy new result to mMergedParcels: cbParcel ----> cbPending
      hasPendingMetaResult = cbPending.vOutputImageItem.size() > 0;
      appendParcel(cbParcel, cbPending);
      break;
    }

    // result from new request comes
    if (!isPending) {
      mMergedParcels.push_back(cbParcel);
    }

    // contains the last partial or not
    for (CallbackParcel::MetaItem mi : cbParcel.vOutputMetaItem) {
      if (mi.bufferNo == mCommonInfo->mAtMostMetaStreamCount) {
        batch->mHasLastPartial.push_back(cbParcel.frameNumber);
      }
    }
    // contains the batched stream or not
    for (CallbackParcel::ImageItem ii : cbParcel.vOutputImageItem) {
      if (ELEMENT_FOUND(mBatchedStreams, ii.buffer->streamId))
        batch->mFrameHasImageResult++;
    }
    // contains the output stream or not. count one while a frame has metadata
    // results
    if (!hasPendingMetaResult && cbParcel.vOutputMetaItem.size() > 0)
      batch->mFrameHasMetaResult++;
  }

  bool isUpdated;
  CallbackParcel cbTemp;
  std::list<CallbackParcel>::iterator itParcel = mMergedParcels.begin();
  itParcel = mMergedParcels.begin();
  while (itParcel != mMergedParcels.end()) {
    // Check there is any callback for this frame
    isUpdated = false;

    CallbackParcel& cbParcel = *itParcel;
    auto batch = getBatchLocked(cbParcel.frameNumber);

    MY_LOGD_IF(
        getLogLevel() >= 2,
        "updated parcel frameNumber:%d - output meta:%zu input image:%zu "
        "output image:%zu",
        cbParcel.frameNumber, cbParcel.vOutputMetaItem.size(),
        cbParcel.vInputImageItem.size(), cbParcel.vOutputImageItem.size());

    // Shutter timestamp
    {
      if (cbParcel.shutter != NULL &&
          !ELEMENT_FOUND(batch->mShutterReturned, cbParcel.frameNumber)) {
        cbTemp.shutter = cbParcel.shutter;
        batch->mShutterReturned.push_back(cbParcel.frameNumber);
        isUpdated = true;
      }
    }

    // Output Metadata
    if (cbParcel.vOutputMetaItem.size() > 0) {
      // All the last partial has arrived, send all the remaining meta
      if ((batch->mHasLastPartial.size() == batch->mBatchSize) ||
          (batch->mFrameHasMetaResult >= batch->mBatchSize) ||
          (ELEMENT_FOUND(batch->mFrameRemoved, cbParcel.frameNumber))) {
        cbTemp.vOutputMetaItem = cbParcel.vOutputMetaItem;
        cbParcel.vOutputMetaItem.clear();
        isUpdated = true;
      }
    }
    // Input Image
    if (cbParcel.vInputImageItem.size() > 0) {
      cbTemp.vInputImageItem = cbParcel.vInputImageItem;
      cbParcel.vInputImageItem.clear();
      isUpdated = true;
    }

    // Output Image
    if (cbParcel.vOutputImageItem.size() > 0) {
      bool hasNonBatchedStream = false;
      Vector<CallbackParcel::ImageItem>::iterator itImage =
          cbParcel.vOutputImageItem.begin();
      while (itImage && itImage != cbParcel.vOutputImageItem.end()) {
        if (!ELEMENT_FOUND(mBatchedStreams, (*itImage).buffer->streamId)) {
          hasNonBatchedStream = true;
          break;
        }
        itImage++;
      }

      if (hasNonBatchedStream ||
          (batch->mFrameHasImageResult >= batch->mBatchSize) ||
          (ELEMENT_FOUND(batch->mFrameRemoved, cbParcel.frameNumber))) {
        if (cbParcel.vError.size() > 0) {
          cbTemp.vError = cbParcel.vError;
          cbParcel.vError.clear();
        }
        cbTemp.vOutputImageItem = cbParcel.vOutputImageItem;
        cbParcel.vOutputImageItem.clear();
        isUpdated = true;
      }
    }

    if (isUpdated || mPendingNonBatchParcels.size() > 0) {
      cbTemp.valid = cbParcel.valid;
      cbTemp.frameNumber = cbParcel.frameNumber;
      cbTemp.needIgnore = cbParcel.needIgnore;
      while (mPendingNonBatchParcels.size() > 0 &&
             mPendingNonBatchParcels[0].frameNumber < cbParcel.frameNumber) {
        MY_LOGD("Callback non-batch result frameNumber:%d",
                mPendingNonBatchParcels[0].frameNumber);
        rUpdatedList.push_back(mPendingNonBatchParcels[0]);
        mPendingNonBatchParcels.removeAt(0);
      }
      rUpdatedList.push_back(cbTemp);
      cbTemp = CallbackParcel();
    }

    if (cbParcel.vInputImageItem.isEmpty() &&
        cbParcel.vOutputImageItem.isEmpty() &&
        cbParcel.vOutputMetaItem.isEmpty() && cbParcel.vError.isEmpty() &&
        ELEMENT_FOUND(batch->mFrameRemoved, cbParcel.frameNumber)) {
      itParcel = mMergedParcels.erase(itParcel);
      batch->mFrameRemovedCount++;

      if (batch->mFrameRemovedCount == batch->mBatchSize)
        removeBatchLocked(batch->mBatchNo);
    } else {
      itParcel++;
    }
  }

  while (mPendingNonBatchParcels.size() > 0) {
    MY_LOGD("Callback non-batch result frameNumber:%d",
            mPendingNonBatchParcels[0].frameNumber);
    rUpdatedList.push_back(mPendingNonBatchParcels[0]);
    mPendingNonBatchParcels.removeAt(0);
  }
#undef ELEMENT_FOUND
}
};  // namespace core
};  // namespace mcam
