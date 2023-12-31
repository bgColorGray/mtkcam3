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

################################################################################
# headers
################################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libmtkcam_devicemgr_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../../../..
include $(MTKCAM_BUILD_HEADER_LIBRARY)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
# ifneq (,$(filter $(strip $(MTKCAM_TARGET_BOARD_PLATFORM)), mt6739 mt8168 mt6765 mt6761 mt6771 mt6779 mt6768))
MTKCAM_MAX_NUM_OF_MULTI_OPEN    ?= 1    # max. number of multi-open cameras
# else
# MTKCAM_MAX_NUM_OF_MULTI_OPEN    ?= 2    # max. number of multi-open cameras
# endif
MTKCAM_HAVE_METADATAPROVIDER    ?= '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_SENSOR_HAL          ?= '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_FLASH_HAL           ?= '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_CAM_MANAGER         ?= '1'  # built-in if '1' ; otherwise not built-in

#-----------------------------------------------------------
LOCAL_SRC_FILES += CameraDeviceManagerImpl.cpp

#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += libmtkcam_headers libmtkcam3_headers
#LOCAL_HEADER_LIBRARIES += libmtkcam_hal3a_headers
LOCAL_HEADER_LIBRARIES += libmtkcam_devicemgr_headers

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-devicemgr"'
#
LOCAL_CFLAGS += -DMTKCAM_MAX_NUM_OF_MULTI_OPEN="$(MTKCAM_MAX_NUM_OF_MULTI_OPEN)"
LOCAL_CFLAGS += -DMTKCAM_HAVE_METADATAPROVIDER="$(MTKCAM_HAVE_METADATAPROVIDER)"
LOCAL_CFLAGS += -DMTKCAM_HAVE_SENSOR_HAL="$(MTKCAM_HAVE_SENSOR_HAL)"
LOCAL_CFLAGS += -DMTKCAM_HAVE_FLASH_HAL="$(MTKCAM_HAVE_FLASH_HAL)"
LOCAL_CFLAGS += -DMTKCAM_HAVE_CAM_MANAGER="$(MTKCAM_HAVE_CAM_MANAGER)"
#
ifeq ($(MTK_CAM_NATIVE_PIP_SUPPORT),yes)
LOCAL_CFLAGS += -DMTKCAM_HAVE_NATIVE_PIP
endif
#

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
LOCAL_STATIC_LIBRARIES += libmtkcam_devicemgrbase

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libladder
#
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
#LOCAL_SHARED_LIBRARIES += libmtkcam_metaconv
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libmtkcam_debugutils
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
LOCAL_SHARED_LIBRARIES += libcamera_metadata
LOCAL_SHARED_LIBRARIES += libfeatureiodrv_mem
#LOCAL_SHARED_LIBRARIES += libmtkcam.logicalcaminfoprovider
LOCAL_SHARED_LIBRARIES += libmtkcam_diputils	#for DipUtils
LOCAL_SHARED_LIBRARIES += libcam.halsensor
#LOCAL_SHARED_LIBRARIES += libcam.hal3a.v3.strobe
#
ifeq "'1'" "$(strip $(MTKCAM_HAVE_METADATAPROVIDER))"
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
else
$(warning "warning: FIXME: MTKCAM_HAVE_METADATAPROVIDER=0")
endif
#
ifeq "'1'" "$(strip $(MTKCAM_HAVE_CAM_MANAGER))"
LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils
#LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils_android
else
$(warning "warning: FIXME: MTKCAM_HAVE_CAM_MANAGER=0")
endif
#

#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam_devicemgr
LOCAL_MODULE_OWNER := mtk
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MULTILIB := first
#-----------------------------------------------------------
include $(MTKCAM_SHARED_LIBRARY)


################################################################################
#
################################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))

