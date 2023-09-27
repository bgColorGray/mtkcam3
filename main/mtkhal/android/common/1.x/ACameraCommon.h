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
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#ifndef MAIN_MTKHAL_ANDROID_COMMON_1_X_ACAMERACOMMON_H_
#define MAIN_MTKHAL_ANDROID_COMMON_1_X_ACAMERACOMMON_H_

#include <mtkcam3/main/mtkhal/android/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/android/device/3.x/types.h>
#include <mtkcam3/main/mtkhal/android/provider/2.x/types.h>
#include <mtkcam3/main/mtkhal/core/common/1.x/types.h>
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/types.h>
#include <mtkcam/def/ImageFormat.h>
#include <mtkcam/utils/metadata/IMetadata.h>

#include <string>
#include <vector>

using NSCam::IMetadata;

namespace mcam {
namespace android {
/******************************************************************************
 *
 ******************************************************************************/
auto convertStatus(mcam::Status status) -> Status;

auto convertStreamConfiguration(const StreamConfiguration& srcStreams,
                                mcam::StreamConfiguration& dstStreams)
    -> int;

auto convertStreams(const std::vector<Stream>& srcStreams,
                    std::vector<mcam::Stream>& dstStreams) -> int;

auto convertStream(const Stream& srcStreams, mcam::Stream& dstStreams)
    -> int;

auto convertStreamRotation(const StreamRotation& srcStreamRotation,
                           uint32_t& dstTransform) -> int;

auto convertHalStreamConfiguration(
    const mcam::HalStreamConfiguration& srcStreams,
    HalStreamConfiguration& dstStreams) -> int;

auto convertHalStreams(const std::vector<mcam::HalStream>& srcStreams,
                       std::vector<HalStream>& dstStreams) -> int;

auto convertHalStream(const mcam::HalStream& srcStreams,
                      HalStream& dstStreams) -> int;

auto convertCaptureRequests(
    const std::vector<CaptureRequest>& srcRequests,
    std::vector<mcam::CaptureRequest>& dstRequests) -> int;

auto convertCaptureRequest(const CaptureRequest& srcRequest,
                           mcam::CaptureRequest& dstRequest) -> int;

auto convertMetadata(const camera_metadata* const& srcMetadata,
                     IMetadata& dstMetadata) -> int;

auto convertMetadata(const IMetadata& srcMetadata,
                     const camera_metadata*& dstMetadata) -> int;

auto convertStreamBuffers(const std::vector<StreamBuffer>& srcBuffer,
                          std::vector<mcam::StreamBuffer>& dstBuffer)
    -> int;

auto convertStreamBuffers(
    const std::vector<mcam::StreamBuffer>& srcBuffer,
    std::vector<StreamBuffer>& dstBuffer) -> int;

auto convertStreamBuffer(const StreamBuffer& srcBuffer,
                         mcam::StreamBuffer& dstBuffer) -> int;

auto convertStreamBuffer(const mcam::StreamBuffer& srcBuffer,
                         StreamBuffer& dstBuffer) -> int;

auto convertCameraResourceCost(const mcam::CameraResourceCost& srcCost,
                               CameraResourceCost& dstCost) -> int;

auto convertExtConfigurationResults(
    const mcam::ExtConfigurationResults& srcResults,
    ExtConfigurationResults& dstResults) -> int;

auto convertCaptureResults(
    const std::vector<mcam::CaptureResult>& srcResults,
    std::vector<CaptureResult>& dstResults,
    int32_t maxMetaCount) -> int;

auto convertCaptureResult(const mcam::CaptureResult& srcResult,
                          CaptureResult& dstResult) -> int;

auto convertPhysicalCameraMetadata(
    const mcam::PhysicalCameraMetadata& srcResult,
    PhysicalCameraMetadata& dstResult) -> int;

auto convertNotifyMsgs(const std::vector<mcam::NotifyMsg>& srcMsgs,
                       std::vector<NotifyMsg>& dstMsgs) -> int;

auto convertNotifyMsg(const mcam::NotifyMsg& srcMsg, NotifyMsg& dstMsg)
    -> int;

auto convertShutterMsg(const mcam::ShutterMsg& srcMsg, NotifyMsg& dstMsg)
    -> void;

auto convertErrorMsg(const mcam::ErrorMsg& srcMsg, NotifyMsg& dstMsg)
    -> void;

#if 0  // not implement HAL Buffer Management
auto convertBufferRequests(
    const std::vector<mcam::BufferRequest>& srcRequests,
    std::vector<BufferRequest>& dstRequests) -> int;

auto convertBufferRequest(const mcam::BufferRequest& srcRequest,
                          BufferRequest& dstRequest) -> int;

auto convertStreamBufferRets(
    const std::vector<mcam::StreamBufferRet>& srcRets,
    std::vector<StreamBufferRet>& dstRets) -> int;

auto convertStreamBufferRet(const mcam::StreamBufferRet& srcBufferRet,
                            StreamBufferRet& dstBufferRet) -> int;

auto convertStreamBuffersVal(const mcam::StreamBuffersVal& srcBuffersVal,
                             StreamBuffersVal& dstBuffersVal) -> int;
#endif

auto convertCameraIdAndStreamCombinations(
    const std::vector<CameraIdAndStreamCombination>& srcConfigs,
    std::vector<mcam::CameraIdAndStreamCombination>& dstConfigs) -> int;

auto convertCameraIdAndStreamCombination(
    const CameraIdAndStreamCombination& srcConfigs,
    mcam::CameraIdAndStreamCombination& dstConfigs) -> int;

auto dumpMetadata(const IMetadata& srcMetadata, int frameNumber = -1) -> int;

auto freeCameraMetadata(const std::vector<CaptureResult>& captureResults)
    -> void;

auto printAutoTestLog(const camera_metadata_t* pMetadata, bool bIsOutput)
    -> void;

};  // namespace android
};  // namespace mcam

#endif  // MAIN_MTKHAL_ANDROID_COMMON_1_X_ACAMERACOMMON_H_
