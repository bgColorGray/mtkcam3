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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#ifndef VENDOR_MEDIATEK_HARDWARE_CAMERA3_MAIN_HAL_SECURITY_GRAPHICS_TYPES_H
#define VENDOR_MEDIATEK_HARDWARE_CAMERA3_MAIN_HAL_SECURITY_GRAPHICS_TYPES_H

#include <android/hardware/graphics/common/1.2/types.h>
#include <gm/gm_buffer_usage.h>

// ------------------------------------------------------------------------

namespace NSCam {

using ::android::hardware::graphics::common::V1_2::BufferUsage;
using ::mediatek::graphics::common::GM_BUFFER_USAGE_PRIVATE_CAMERA;

// BufferUsage flag combination definitions
enum class MTKCam_BufferUsage : uint64_t {
    /* ImageReader case */
    PROT_CAMERA_OUTPUT = BufferUsage::PROTECTED | BufferUsage::CAMERA_OUTPUT,
    /* ImageWriter case */
    PROT_CAMERA_INPUT = BufferUsage::PROTECTED | BufferUsage::CAMERA_INPUT,
    /* ImageReader and ImageWriter case */
    PROT_CAMERA_BIDIRECTIONAL = PROT_CAMERA_OUTPUT | PROT_CAMERA_INPUT,
    /*
     * NOTE: Please use buffer usage mentioned above for camera image buffers.
     *       PROT_CAMERA_LEGACY is used for trusted drivers' working buffer in TrustZone.
     *
     * bits 28-31 are reserved for vendor extensions
     * Reference: hardware/interfaces/graphics/common/1.0/types.hal
     */
    VENDOR_EXTENSION_2 = GM_BUFFER_USAGE_PRIVATE_CAMERA,
    /* Legacy camera HAL3 secure buffer heap */
    PROT_CAMERA_LEGACY = BufferUsage::PROTECTED | VENDOR_EXTENSION_2,
};

// make sure that AOSP buffer usage is 64-bit unsigned integer value type.
static_assert(std::is_same<std::underlying_type_t<BufferUsage>, std::uint64_t>::value,
                           "wrong value type");
// make sure that the value type of MTK camera buffer usage is the same as that
// of AOSP buffer usage.
static_assert(std::is_same<std::underlying_type_t<MTKCam_BufferUsage>, std::underlying_type_t<BufferUsage>>::value,
                           "mispatched value type");

} // namespace NSCam

#endif // VENDOR_MEDIATEK_HARDWARE_CAMERA3_MAIN_HAL_SECURITY_GRAPHICS_TYPES_H
