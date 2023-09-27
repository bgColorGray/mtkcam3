# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2015. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.

################################################################################
#
################################################################################
ifeq ($(MTK_CAM_STEREO_CAMERA_SUPPORT), yes)

LOCAL_PATH := $(call my-dir)
PLATFORM := $(MTKCAM_PLATFORM_DIR)
include $(CLEAR_VARS)
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/drv/driver.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/effectHal.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/common/vsdof/vsdof_common.mk
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-vsdof"'
LOCAL_CFLAGS += -DTARGET_BUILD_VARIANT
################################################################################
ifneq ($(MTKCAM_IP_BASE),0)
ifneq ($(MTK_BASIC_PACKAGE), yes)

## buffer manager
LOCAL_SRC_FILES += ./bufferPoolMgr/bufferSize/BaseBufferSizeMgr.cpp
LOCAL_SRC_FILES += ./bufferPoolMgr/bufferSize/NodeBufferSizeMgr.cpp
LOCAL_SRC_FILES += ./bufferPoolMgr/NodeBufferHandler.cpp
LOCAL_SRC_FILES += ./bufferPoolMgr/NodeBufferPoolMgr_VSDOF.cpp
LOCAL_SRC_FILES += ./bufferConfig/BaseBufferConfig.cpp
## driver interface
LOCAL_SRC_FILES += ./driverInterface/DPEDrv.cpp
## flow option
LOCAL_SRC_FILES += ./flowOption/qparams/DepthQTemplateProvider.cpp
LOCAL_SRC_FILES += ./flowOption/DepthMapFlowOption_VSDOF.cpp
## nodes
LOCAL_SRC_FILES += ./nodes/P2ANode.cpp
LOCAL_SRC_FILES += ./nodes/P2ABayerNode.cpp
LOCAL_SRC_FILES += ./nodes/N3DNode.cpp
LOCAL_SRC_FILES += ./nodes/DVSNode.cpp
LOCAL_SRC_FILES += ./nodes/DVPNode.cpp
LOCAL_SRC_FILES += ./nodes/GFNode.cpp
LOCAL_SRC_FILES += ./nodes/NR3DCommon.cpp
LOCAL_SRC_FILES += ./nodes/WPENode.cpp
LOCAL_SRC_FILES += ./nodes/DLDepthNode.cpp
LOCAL_SRC_FILES += ./nodes/SlantNode.cpp
## pipe
LOCAL_SRC_FILES += DepthMapEffectRequest.cpp
LOCAL_SRC_FILES += DepthMapPipeNode.cpp
LOCAL_SRC_FILES += DepthMapPipe.cpp
LOCAL_SRC_FILES += DepthMapPipe_Common.cpp
LOCAL_SRC_FILES += DepthMapPipeUtils.cpp
LOCAL_SRC_FILES += DepthMapFactory.cpp
LOCAL_SRC_FILES += DepthMapPipeTimer.cpp

endif
endif

# ##############################################################################
LOCAL_HEADER_LIBRARIES += $(MTKCAM_INCLUDE_HEADER_LIB)
LOCAL_HEADER_LIBRARIES += libmtkcam_headers
LOCAL_HEADER_LIBRARIES += libmtkcam3_headers
LOCAL_HEADER_LIBRARIES += libcameracustom_headers
LOCAL_HEADER_LIBRARIES += libmtkcam_algorithm_headers
LOCAL_HEADER_LIBRARIES += device_kernel_headers
LOCAL_SHARED_LIBRARIES += libaedv
LOCAL_HEADER_LIBRARIES += libaed_headers
# mtkcam3/feature/core/featurePipe/core/include
LOCAL_HEADER_LIBRARIES += libmtkcam_featurepipe_core_headers
# mtkcma3/feature/include
LOCAL_HEADER_LIBRARIES += libmtkcam3_feautre_headers
# feature/common/vsdof/providers
LOCAL_HEADER_LIBRARIES += libstereoprovider_headers

################################################################################
LOCAL_WHOLE_STATIC_LIBRARIES += libmtkcam.featurepipe.core
LOCAL_WHOLE_STATIC_LIBRARIES += libfeature.vsdof.common
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libnativewindow
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libcam.iopipe
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils libmtkcam_imgbuf
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
LOCAL_SHARED_LIBRARIES += libfeature.vsdof.hal
LOCAL_SHARED_LIBRARIES += libcamalgo.fdft
ifeq ($(INDEP_STEREO_PROVIDER), true)
    LOCAL_SHARED_LIBRARIES += libfeature.stereo.provider
endif
LOCAL_SHARED_LIBRARIES += libeffecthal.base
LOCAL_SHARED_LIBRARIES += libgralloc_extra
LOCAL_SHARED_LIBRARIES += libfeature.face
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam.featurepipe.vsdof_util
LOCAL_SHARED_LIBRARIES += libcameracustom
# IMetadataProvider
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
# LOCAL_SHARED_LIBRARIES += libmtkcam_dpe
LOCAL_SHARED_LIBRARIES += libmtkcam_owe
LOCAL_SHARED_LIBRARIES += libfeature_3dnr
LOCAL_SHARED_LIBRARIES += libJpgEncPipe
LOCAL_SHARED_LIBRARIES += libmtkcam_exif
# for fence
LOCAL_SHARED_LIBRARIES += libsync
# DpBlitStream
LOCAL_SHARED_LIBRARIES += libdpframework
# for ion
LOCAL_SHARED_LIBRARIES += libion
LOCAL_SHARED_LIBRARIES += libion_mtk
# for dump
LOCAL_SHARED_LIBRARIES += libmtkcam_tuning_utils
LOCAL_SHARED_LIBRARIES += libaedv
#
LOCAL_SHARED_LIBRARIES += libcam.feature_utils
#
LOCAL_SHARED_LIBRARIES += libladder
LOCAL_SHARED_LIBRARIES += libfeatureiodrv_mem
# hw util (FD Container)
LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils
# slant camera - sw rotate
LOCAL_SHARED_LIBRARIES += libcamalgo.rotate

################################################################################
# LOCAL_CFLAGS += -DGTEST
# LOCAL_CFLAGS += -DGTEST_PROFILE
#LOCAL_CFLAGS += -DGTEST_PARTIAL
LOCAL_CFLAGS += -DUNDER_DEVELOPMENT
################################################################################
LOCAL_MODULE := libmtkcam.featurepipe.depthmap
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTKCAM_SHARED_LIBRARY)
#include $(MTKCAM_STATIC_LIBRARY)
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
################################################################################

endif #MTK_CAM_STEREO_CAMERA_SUPPORT