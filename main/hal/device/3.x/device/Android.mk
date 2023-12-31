# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2020. All rights reserved.
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

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/mtkcam_option.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
#

#-----------------------------------------------------------
LOCAL_SRC_FILES += CameraCommon.cpp
LOCAL_SRC_FILES += CameraDevice3Factory.cpp
LOCAL_SRC_FILES += CameraDevice3Impl.cpp
LOCAL_SRC_FILES += CameraDevice3SessionFactory.cpp
LOCAL_SRC_FILES += CameraDevice3SessionImpl.cpp
LOCAL_SRC_FILES += Camera3OfflineSessionFactory.cpp
LOCAL_SRC_FILES += Camera3OfflineSessionImpl.cpp
LOCAL_SRC_FILES += ZoomRatioConverter.cpp
LOCAL_SRC_FILES += CustConfigTimeCPUCtrl.cpp

#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += $(MTKCAM_INCLUDE_HEADER_LIB)
LOCAL_HEADER_LIBRARIES += libmtkcam_headers
LOCAL_HEADER_LIBRARIES += libladder_headers
LOCAL_HEADER_LIBRARIES += mtk_openmax_headers
LOCAL_HEADER_LIBRARIES += libnativewindow_headers
LOCAL_HEADER_LIBRARIES += libarect_headers
LOCAL_HEADER_LIBRARIES += libnativebase_headers
LOCAL_HEADER_LIBRARIES += libmtkcam_device3_headers

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-dev3"'
#
ifeq ($(TARGET_BUILD_VARIANT),user)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="0"
endif
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="1"
endif
ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_CFLAGS += -DMTKCAM_TARGET_BUILD_VARIANT="2"
endif

ifeq ($(MTKCAM_OPT_CAM_DEV_TIMEOUT),500)
    LOCAL_CFLAGS += -DCPU_TIMEOUT=$(MTKCAM_OPT_CAM_DEV_TIMEOUT)
else
    LOCAL_CFLAGS += -DCPU_TIMEOUT=1000
endif

ifneq (,$(filter $(strip $(MTKCAM_TARGET_BOARD_PLATFORM)), mt6855 mt6789))
  LOCAL_CFLAGS += -DSUPPORT_ASYNC_CLOSING
endif

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
#
# LOCAL_STATIC_LIBRARIES += libmtkcam_device3-common-static
#
#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += libpowerhal_headers
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libhardware
LOCAL_SHARED_LIBRARIES += libcamera_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils

# LOCAL_SHARED_LIBRARIES += libnativewindow
#
# LOCAL_SHARED_LIBRARIES += android.hardware.power@1.0
# LOCAL_SHARED_LIBRARIES += vendor.mediatek.hardware.power@2.0
LOCAL_SHARED_LIBRARIES += libmtkcam_sysutils
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
LOCAL_SHARED_LIBRARIES += libmtkcam_devicesessionpolicy
#-----------------------------------------------------------
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam_device3_hal-base-static
LOCAL_MODULE_OWNER := mtk
LOCAL_PROPRIETARY_MODULE := true

#-----------------------------------------------------------
include $(MTKCAM_STATIC_LIBRARY)


################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
LOCAL_SRC_FILES +=

#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += $(MTKCAM_INCLUDE_HEADER_LIB)
LOCAL_HEADER_LIBRARIES += libmtkcam_headers
LOCAL_HEADER_LIBRARIES += libladder_headers
#

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-dev3"'

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES += libmtkcam_device3_hal-base-static
#
# LOCAL_STATIC_LIBRARIES += libmtkcam_device3-common-static

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += libmtkcam_device3_utils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libladder
LOCAL_SHARED_LIBRARIES += libcamera_metadata
#
LOCAL_SHARED_LIBRARIES += libmtkcam_sysutils
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
# LOCAL_SHARED_LIBRARIES += libmtkcam_debugutils
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libmtkcam_device3_app
LOCAL_SHARED_LIBRARIES += libmtkcam_devicesessionpolicy
LOCAL_SHARED_LIBRARIES += libmtkcam_pipelinemodel
#
# LOCAL_SHARED_LIBRARIES += android.hardware.power@1.0
# LOCAL_SHARED_LIBRARIES += vendor.mediatek.hardware.power@2.0

# HwInfoHelper
LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils
#-----------------------------------------------------------
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam_device3_hal
LOCAL_MODULE_OWNER := mtk
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MULTILIB := first
#-----------------------------------------------------------
include $(MTKCAM_SHARED_LIBRARY)


################################################################################
#
################################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))
