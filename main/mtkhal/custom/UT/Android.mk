# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2010. All rights reserved.
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

LOCAL_PATH := $(call my-dir)

# -----------------------------------------------------------------------------
# Header Library
# -----------------------------------------------------------------------------
include $(CLEAR_VARS)

# headers
################################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libmtkcam_hal_ut_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../../../..
include $(MTKCAM_BUILD_HEADER_LIBRARY)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

ifneq ($(MYKCAM_USE_LEGACY_HAL_API),yes)
#-----------------------------------------------------------
LOCAL_SRC_FILES += UnitTest.cpp
#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-camprovider-test"'
#
ifeq ($(TARGET_BUILD_VARIANT),user)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="'u'"
endif
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="'d'"
endif
ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="'e'"
endif

#-----------------------------------------------------------
LOCAL_STATIC_LIBRARIES += libjsoncpp

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.4
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.5
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.6
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.2
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.4
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.5
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.6
LOCAL_SHARED_LIBRARIES += android.hardware.graphics.allocator@4.0
LOCAL_SHARED_LIBRARIES += android.hardware.graphics.mapper@4.0
LOCAL_SHARED_LIBRARIES += android.hardware.graphics.common@1.2
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcamera_metadata
LOCAL_SHARED_LIBRARIES += libfmq
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libnativewindow
LOCAL_SHARED_LIBRARIES += libhardware
LOCAL_SHARED_LIBRARIES += libhidlbase
LOCAL_SHARED_LIBRARIES += libhidltransport
LOCAL_SHARED_LIBRARIES += libfmq
LOCAL_SHARED_LIBRARIES += libui
LOCAL_SHARED_LIBRARIES += libion
LOCAL_SHARED_LIBRARIES += libion_mtk
LOCAL_SHARED_LIBRARIES += libgralloc_extra
#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += libladder_headers
# LOCAL_SHARED_LIBRARIES += libmtkcam_device3_hidl
LOCAL_HEADER_LIBRARIES += libmtkcam_headers libmtkcam3_headers
# LOCAL_HEADER_LIBRARIES += libmtkcam_device3_hidl_tinymw_headers
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
# LOCAL_SHARED_LIBRARIES += libmtkcam_metaconv
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libcamalgo.fdft
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
# LOCAL_SHARED_LIBRARIES += libmtkcam_device3_custom_graphics
# LOCAL_SHARED_LIBRARIES += libmtkcam_device3_custom
LOCAL_SHARED_LIBRARIES += libmtkcam_postprocprovider
#LOCAL_SHARED_LIBRARIES += libmtkcam_hal_custom
#LOCAL_SHARED_LIBRARIES += libmtkcam_hal_custom_provider
LOCAL_SHARED_LIBRARIES += libmtkcam_hal_core_provider
LOCAL_SHARED_LIBRARIES += libmtkcam_imgbuf_v2
LOCAL_HEADER_LIBRARIES += mtkcam_interfaces_headers
LOCAL_HEADER_LIBRARIES += libmtkcam_android_base_folder_header
LOCAL_HEADER_LIBRARIES += libmtkcam_hal_ut_headers

#-----------------------------------------------------------
LOCAL_MULTILIB := first
LOCAL_MODULE := libmtkcam_hal_ut
LOCAL_MODULE_OWNER := mtk
LOCAL_PROPRIETARY_MODULE := true
#-----------------------------------------------------------
include $(MTKCAM_SHARED_LIBRARY)


################################################################################
#
################################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))
endif # ifneq ($(MYKCAM_USE_LEGACY_HAL_API),yes)
