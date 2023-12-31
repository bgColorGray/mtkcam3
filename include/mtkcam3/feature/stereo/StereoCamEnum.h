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

#ifndef _MTK_LEGACYPIPELINE_STEREO_CAM_COMMON_H_
#define _MTK_LEGACYPIPELINE_STEREO_CAM_COMMON_H_

namespace NSCam {
namespace v1 {
namespace Stereo {

enum StereoFeatureMode
{
    E_INIT_MODE = 0,
    E_STEREO_FEATURE_CAPTURE       = 1,
    E_STEREO_FEATURE_VSDOF         = 1<<2,
    E_STEREO_FEATURE_DENOISE       = 1<<3,
    E_STEREO_FEATURE_THIRD_PARTY   = 1<<4,
    E_DUALCAM_FEATURE_ZOOM         = 1<<5,
    E_STEREO_FEATURE_MTK_DEPTHMAP  = 1<<6,
    E_STEREO_FEATURE_ACTIVE_STEREO = 1<<7,
    E_STEREO_FEATURE_MULTI_CAM     = 1<<8,
    E_SECURE_CAMERA                = 1<<9,
    E_MULTICAM_FEATURE_HIDL_FLAG   = 1<<30,
    E_STEREO_FEATURE_PORTRAIT_FLAG = 1<<31,
};

enum SeneorModuleType
{
    INIT_TYPE = 0,
    UNSUPPORTED_SENSOR_MODULE = 1<<6,
    BAYER_AND_BAYER = 1<<7,
    BAYER_AND_MONO = 1<<8,
};

/**
 * This enum needs to match to getCallbackBufferList.
 * details Buffers:
 *       ci: Clean Image
 *       bi: Bokeh Image
 *      rbi: Relighting Bokeh Image
 *      mbd: MTK Bokeh Depth
 *      mdb: MTK Debug Buffer
 *      mbm: MTK Bokeh Metadata
  */
enum CallbackBufferType
{
    E_DUALCAM_JPEG_CLEAN_IMAGE,
    E_DUALCAM_JPEG_BOKEH_IMAGE,
    E_DUALCAM_JPEG_RELIGHTING_BOKEH_IMAGE,
    E_DUALCAM_JPEG_MTK_BOKEH_DEPTH,
    E_DUALCAM_JPEG_MTK_DEBUG_BUFFER,
    E_DUALCAM_JPEG_MTK_BOKEH_METADATA,
    E_DUALCAM_JPEG_ENUM_SIZE,
};
  /**
 * P1 Direct YUV  definitions
 * AS_P1_YUV_CRZ & HAS_P1_YUV_RSSO_R2 are exclusive
 */
enum  DirectYUVType
{
    NO_P1_YUV               = 0,
    HAS_P1_YUV_CRZO         = 1,
    HAS_P1_YUV_RSSO_R2      = 2,
    HAS_P1_YUV_YUVO         = 4,
    //HAS_P1_YUV_YUVO_CRZO  = 5,
    HAS_P1_YUV_YUVO_RSSO_R2 = 6,
};
};
};
};
#endif