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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_DEBUG_CONTROL_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_DEBUG_CONTROL_H_

#define KEY_FORCE_P2G_NO_FAT          "vendor.debug.fpipe.p2g.nofat"
#define KEY_FORCE_EIS                 "vendor.debug.fpipe.force.eis"
#define KEY_FORCE_EIS_30              "vendor.debug.fpipe.force.eis30"
#define KEY_FORCE_EIS_QUEUE           "vendor.debug.fpipe.force.eisq"
#define KEY_FORCE_3DNR                "vendor.debug.fpipe.force.3dnr"
#define KEY_FORCE_3DNR_S              "vendor.debug.fpipe.force.3dnr_s"
#define KEY_FORCE_AOSP_ION            "vendor.debug.fpipe.force.aosp.ion"
#define KEY_FORCE_DSDN20              "vendor.debug.fpipe.force.dsdn20"
#define KEY_FORCE_DSDN20_S            "vendor.debug.fpipe.force.dsdn20_s"
#define KEY_FORCE_DSDN25              "vendor.debug.fpipe.force.dsdn25"
#define KEY_FORCE_DSDN25_S            "vendor.debug.fpipe.force.dsdn25_s"
#define KEY_FORCE_DSDN30              "vendor.debug.fpipe.force.dsdn30"
#define KEY_FORCE_DSDN30_S            "vendor.debug.fpipe.force.dsdn30_s"
#define KEY_FORCE_VHDR                "vendor.debug.fpipe.force.vhdr"
#define KEY_FORCE_WARP_PASS           "vendor.debug.fpipe.force.warp.pass"
#define KEY_FORCE_WARP_P              "vendor.debug.fpipe.force.preview.warp"
#define KEY_FORCE_WARP_R              "vendor.debug.fpipe.force.record.warp"
#define KEY_FORCE_VMDP_P              "vendor.debug.fpipe.force.preview.vmdp"
#define KEY_FORCE_VMDP_R              "vendor.debug.fpipe.force.record.vmdp"
#define KEY_FORCE_GPU_OUT             "vendor.debug.fpipe.force.gpu.out"
#define KEY_FORCE_GPU_RGBA            "vendor.debug.fpipe.force.gpu.rgba"
#define KEY_FORCE_IMG3O               "vendor.debug.fpipe.force.img3o"
#define KEY_FORCE_BUF                 "vendor.debug.fpipe.force.buf"
#define KEY_FORCE_DUMMY               "vendor.debug.fpipe.force.dummy"
#define KEY_FORCE_TPI_YUV             "vendor.debug.fpipe.force.tpi.yuv"
#define KEY_FORCE_TPI_ASYNC           "vendor.debug.fpipe.force.tpi.async"
#define KEY_FORCE_XNODE               "vendor.debug.fpipe.force.xnode"
#define KEY_FORCE_TH_RELEASE          "vendor.debug.fpipe.force.thread.release"
#define KEY_ENABLE_DUMMY              "vendor.debug.fpipe.enable.dummy"
#define KEY_ENABLE_PURE_YUV           "vendor.debug.fpipe.enable.pureyuv"
#define KEY_USE_PER_FRAME_SETTING     "vendor.debug.fpipe.frame.setting"
#define KEY_DEBUG_DUMP                "vendor.debug.fpipe.force.dump"
#define KEY_DEBUG_DUMP_COUNT          "vendor.debug.fpipe.force.dump.count"
#define KEY_DEBUG_DUMP_BY_RECORDNO    "vendor.debug.fpipe.dump.by.recordno"
#define KEY_FORCE_RSC_TUNING          "vendor.debug.fpipe.force.rsc.tuning"
#define KEY_FORCE_PRINT_IO            "vendor.debug.fpipe.force.printio"
#define KEY_FORCE_PRINT_NEXT_IO       "vendor.debug.fpipe.force.printnextio"
#define KEY_FORCE_PRINT_P2G           "vendor.debug.fpipe.force.printp2g"
#define KEY_FORCE_PRINT_P2SWUTIL      "vendor.debug.fpipe.force.printp2swutil"
#define KEY_FORCE_PRINT_MDP_WRAPPER   "vendor.debug.fpipe.force.printmdp"
#define KEY_ENABLE_SLAVENR            "vendor.debug.fpipe.enable.slavenr"
#define KEY_ENABLE_PHYSICAL_NR        "vendor.debug.fpipe.enable.physicalnr"
#define KEY_ENABLE_SYNC_CTRL          "vendor.debug.fpipe.enable.sync.ctrl"
#define KEY_DEBUG_ENCODE_PREVIEW      "vendor.debug.eis.encodepreview"

#define KEY_DEBUG_TPI                 "vendor.debug.tpi.s"
#define KEY_DEBUG_TPI_LOG             "vendor.debug.tpi.s.log"
#define KEY_DEBUG_TPI_DUMP            "vendor.debug.tpi.s.dump"
#define KEY_DEBUG_TPI_ERASE           "vendor.debug.tpi.s.erase"
#define KEY_DEBUG_TPI_SCAN            "vendor.debug.tpi.s.scan"
#define KEY_DEBUG_TPI_BYPASS          "vendor.debug.tpi.s.bypass"

#define KEY_DEBUG_WARP_ENDTIME_DEC    "vendor.debug.warp.endtime.decrease"

#define STR_ALLOCATE                  "pool allocate:"

#define MAX_TPI_COUNT                 ((MUINT32)3)
#define MAX_WARP_NODE_COUNT           ((MUINT32)2)
#define MAX_VMDP_NODE_COUNT           ((MUINT32)2)
#define DCESO_DELAY_COUNT             ((MUINT32)2)

#define BUF_ALLOC_ALIGNMENT_BIT       ((MUINT32)6) //64 align

#define TPI_RECORD_QUEUE_DELAY_CNT    ((MUINT32)0)

#define USE_YUY2_FULL_IMG             1
#define USE_NV21_FULL_IMG             1
#define USE_WPE_STAND_ALONE           0

#define SIMULATE_MSS              0

#define SUPPORT_3A_HAL            1
#define SUPPORT_ISP_HAL           1
#define SUPPORT_GRAPHIC_BUFFER    1
#define SUPPORT_GPU_YV12          1
#define SUPPORT_GPU_CROP          1
#define SUPPORT_VENDOR_NODE       0
#define SUPPORT_DUMMY_NODE        0
#define SUPPORT_PURE_YUV          0
#define SUPPORT_FAKE_EIS_30       0
#define SUPPORT_FAKE_WPE          0
#if (MTKCAM_FOV_USE_WPE == 1)
#define SUPPORT_WPE               1
#else
#define SUPPORT_WPE               0
#endif

#define SUPPORT_P2AMDP_THREAD_MERGE   0

#define SUPPORT_VENDOR_SIZE           0
#define SUPPORT_VENDOR_FORMAT         0
#define SUPPORT_VENDOR_FULL_FORMAT    eImgFmt_NV21

#define DEBUG_TIMER               1

#define DEV_VFB_READY             0
#define DEV_P2B_READY             0

#define NO_FORCE  0
#define FORCE_ON  1
#define FORCE_OFF 2

#define VAL_FORCE_P2G_NO_FAT        0
#define VAL_FORCE_EIS               0
#define VAL_FORCE_EIS_30            0
#define VAL_FORCE_EIS_QUEUE         0
#define VAL_FORCE_3DNR              0
#define VAL_FORCE_3DNR_S            0
#define VAL_FORCE_DSDN20            0
#define VAL_FORCE_DSDN20_S          0
#define VAL_FORCE_DSDN25            0
#define VAL_FORCE_DSDN25_S          0
#define VAL_FORCE_DSDN30            0
#define VAL_FORCE_DSDN30_S          0
#define VAL_FORCE_VHDR              0
#define VAL_FORCE_WARP_PASS         0
#define VAL_FORCE_GPU_OUT           0
#define VAL_FORCE_GPU_RGBA          (!SUPPORT_GPU_YV12)
#define VAL_FORCE_IMG3O             0
#define VAL_FORCE_BUF               0
#define VAL_FORCE_DUMMY             0
#define VAL_FORCE_TPI_YUV           0
#define VAL_FORCE_TPI_ASYNC         0
#define VAL_FORCE_XNODE             0
#define VAL_DEBUG_DUMP              0
#define VAL_DEBUG_DUMP_COUNT        1
#define VAL_DEBUG_DUMP_BY_RECORDNO  0
#define VAL_USE_PER_FRAME_SETTING   0
#define VAL_FORCE_RSC_TUNING        0
#define VAL_FORCE_PRINT_IO          0
#define VAL_FORCE_PRINT_NEXT_IO     0
#define VAL_ENABLE_SLAVE_NR         1
#define VAL_ENABLE_PHYSICAL_NR      1
#define VAL_FORCE_WARP_P            1
#define VAL_FORCE_WARP_R            1
#define VAL_FORCE_VMDP_P            1
#define VAL_FORCE_VMDP_R            1
#define VAL_DEBUG_ENDTIME_DEC       0
#define VAL_ENABLE_SYNC_CTRL        1
#define VAL_DEBUG_ENCODE_PREVIEW    0

#define TRACE_STREAMING_FEATURE_COMMON  0
#define TRACE_STREAMING_FEATURE_DATA    0
#define TRACE_STREAMING_FEATURE_NODE    0
#define TRACE_STREAMING_FEATURE_IO      0
#define TRACE_STREAMING_FEATURE_PIPE    0
#define TRACE_STREAMING_FEATURE_USAGE   0
#define TRACE_STREAMING_FEATURE_TIMER   0
#define TRACE_ADV_PQ_CTRL               0
#define TRACE_SYNC_CTRL                 0
#define TRACE_IMG_BUFFER_STORE          0
#define TRACE_DSDN_HAL                  0
#define TRACE_ROOT_NODE                 0
#define TRACE_TOF_NODE                  0
#define TRACE_P2A_NODE                  0
#define TRACE_P2A_3DNR                  0
#define TRACE_P2SM_NODE                 0
#define TRACE_P2SW_NODE                 0
#define TRACE_G_P2A_NODE                0
#define TRACE_G_P2SM_NODE               0
#define TRACE_VNR_NODE                  0
#define TRACE_EIS_NODE                  0
#define TRACE_GPU_NODE                  0
#define TRACE_VENDOR_NODE               0
#define TRACE_VMDP_NODE                 0
#define TRACE_DUMMY_NODE                0
#define TRACE_NULL_NODE                 0
#define TRACE_RSC_NODE                  0
#define TRACE_MSS_NODE                  0
#define TRACE_MSF_NODE                  0
#define TRACE_WARP_NODE                 0
#define TRACE_X_NODE                    0
#define TRACE_HELPER_NODE               0
#define TRACE_TRACKING_NODE             0
#define TRACE_QPARAMS_BASE              0
#define TRACE_WARP_BASE                 0
#define TRACE_GPU_WARP                  0
#define TRACE_WPE_WARP                  0
#define TRACE_COOKIE_STORE              0
#define TRACE_NORMAL_STREAM_BASE        0
#define TRACE_P2DIP_STREAM_BASE         0
#define TRACE_MSS_STREAM_BASE           0
#define TRACE_RSC_STREAM_BASE           0
#define TRACE_RSC_TUNING_STREAM         0
#define TRACE_WARP_STREAM_BASE          0
#define TRACE_GPU_WARP_STREAM_BASE      0
#define TRACE_WPE_WARP_STREAM_BASE      0
#define TRACE_MDP_WRAPPER               0
#define TRACE_SMVR_HAL                  0
#define TRACE_VNR_HAL                   0
// dual zoom
#define TRACE_FOV_NODE                  0
#define TRACE_FOVWARP_NODE              0
#define TRACE_FOV_HAL                   0
#define TRACE_P2A_FOV                   0
// dual preview mode
#define TRACE_N3D_NODE                  0
#define TRACE_N3DP2_NODE                0
#define TRACE_EIS_Q_CONTROL             0
#define TRACE_TPI_NODE                  0
#define TRACE_TPI_ASYNC_NODE            0
#define TRACE_TPI_USAGE                 0
#define TRACE_SFP_DEPTH_NODE            0
#define TRACE_SFP_BOKEH_NODE            0
#define TRACE_P2G_PATH_ENGINE           0
#define TRACE_P2G_P2SWUTIL              0
#define TRACE_P2G_P2HWUTIL              0
#define TRACE_P2G_MGR                   0

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_DEBUG_CONTROL_H_

#ifdef PIPE_MODULE_TAG
#undef PIPE_MODULE_TAG
#endif
#define PIPE_MODULE_TAG "MtkCam/StreamingPipe"

