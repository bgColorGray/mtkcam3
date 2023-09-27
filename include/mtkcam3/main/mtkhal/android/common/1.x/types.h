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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_COMMON_1_X_TYPES_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_COMMON_1_X_TYPES_H_

#include <cerrno>
#include <vector>
#include <string>

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace android {

enum class Status : uint32_t {
  OK = 0,
  ILLEGAL_ARGUMENT = 1,
  CAMERA_IN_USE = 2,
  MAX_CAMERAS_IN_USE = 3,
  METHOD_NOT_SUPPORTED = 4,
  OPERATION_NOT_SUPPORTED = 5,
  CAMERA_DISCONNECTED = 6,
  INTERNAL_ERROR = 7
};

/**
 * Possible states that the flash unit on a closed camera device can be set to
 * via the ICameraProvider::setTorchMode() method.
 */
enum class TorchMode : uint32_t {
  OFF = 0,  // Turn off the flash
  ON = 1    // Turn on the flash to torch mode
};

enum class CameraMetadataType : uint32_t {
  BYTE = 0,     // Unsigned 8-bit integer (uint8_t)
  INT32 = 1,    // Signed 32-bit integer (int32_t)
  FLOAT = 2,    // 32-bit float (float)
  INT64 = 3,    // Signed 64-bit integer (int64_t)
  DOUBLE = 4,   // 64-bit float (double)
  RATIONAL = 5  // A 64-bit fraction (camera_metadata_rational_t)
};

/**
 * A single vendor-unique metadata tag.
 * The full name of the tag is <sectionName>.<tagName>
 */
struct VendorTag {
  uint32_t tagId;       // Tag identifier, must be >= TagBoundaryId::VENDOR
  std::string tagName;  // Name of tag, not including section name
  CameraMetadataType tagType;
};

/**
 * A set of related vendor tags.
 */
struct VendorTagSection {
  std::string
      sectionName;  // Section name; must be namespaced within vendor's name
  std::vector<VendorTag> tags;  // List of tags in this section
};

enum class TagBoundaryId : uint32_t {
  AOSP = 0x0,           // First valid tag id for android-defined tags
  VENDOR = 0x80000000u  // First valid tag id for vendor extension tags
};

enum class CameraDeviceStatus : uint32_t {
  NOT_PRESENT = 0,  // not currently connected
  PRESENT = 1,      // connected
  ENUMERATING = 2,  // undergoing enumeration
};

enum class TorchModeStatus : uint32_t {
  NOT_AVAILABLE = 0,  // the torch mode can not be turned on
  AVAILABLE_OFF =
      1,  // A torch mode has become off and is available to be turned on
  AVAILABLE_ON =
      2,  // A torch mode has become on and is available to be turned off
};

struct CameraResourceCost {
  uint32_t resourceCost = 0;
  std::vector<int32_t> conflictingDevices;
};

};      // namespace android
};      // namespace mcam

#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_ANDROID_COMMON_1_X_TYPES_H_
