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
 * MediaTek Inc. (C) 2021. All rights reserved.
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
#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_GRALLOC_1_X_IGRALLOCINFOPROVIDER_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_GRALLOC_1_X_IGRALLOCINFOPROVIDER_H_

#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {


/**
 * The current stream information is used to query the gralloc buffer information.
 */
struct GrallocRequest {

    /**
     * The gralloc usage, possible values were defined at:
     * https://android.googlesource.com/platform/hardware/libhardware/+/refs/heads/master/include/hardware/gralloc.h#68
     */
    int                     usage;

    /**
     * The image format to request, possible values were defined at:
     * https://android.googlesource.com/platform/system/core/+/refs/heads/master/libsystem/include/system/graphics-base-v1.0.h
     * and the later versions.
     */
    int                     format;

    /**
     * The image width in pixels. For formats where some color channels are
     * subsampled, this is the width of the largest-resolution plane.
     */
    int                     widthInPixels;

    /**
     * The image height in pixels. For formats where some color channels are
     * subsampled, this is the height of the largest-resolution plane.
     */
    int                     heightInPixels;

};


/**
 * The static information of buffer.
 */
struct GrallocStaticInfo {
    /**
     * The static information of buffer plane.
     */
    struct Plane {
        /**
         * The size for this color plane, in bytes.
         */
        size_t              sizeInBytes;

        /**
         * The row stride for this color plane, in bytes.
         *
         * This is the distance between the start of two consecutive rows of
         * pixels in the image. The row stride is always greater than 0.
         */
        size_t              rowStrideInBytes;

    };

    /**
     * The resulting image format.
     */
    int                     format;

    /**
     * The image width in pixels. For formats where some color channels are
     * subsampled, this is the width of the largest-resolution plane.
     */
    int                     widthInPixels;

    /**
     * The image height in pixels. For formats where some color channels are
     * subsampled, this is the height of the largest-resolution plane.
     */
    int                     heightInPixels;

    /**
     * A vector of planes.
     */
    std::vector<Plane>  planes;

};


/**
 *
 */
class IGrallocInfoProvider {
public:     ////                Instantiation.
    virtual                     ~IGrallocInfoProvider() {}
    /**
     * Get a singleton instance.
     */
    static  IGrallocInfoProvider*     getInstance();

public:     ////                Interfaces.
    /**
     * Given a undefined gralloc foramt, such as IMPLEMENTATION_DEFINED or
     * YUV_420_888, return its information.
     */
    virtual int                 query(
                                    struct GrallocRequest const*    pRequest,
                                    struct GrallocStaticInfo*       pStaticInfo,
                                    bool bReconfig = false
                                ) const                                     = 0;

};

}  // namespace mcam

#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_UTILS_GRALLOC_1_X_IGRALLOCINFOPROVIDER_H_
