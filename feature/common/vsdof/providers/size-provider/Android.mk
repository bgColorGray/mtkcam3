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
# ifeq ($(MTK_CAM_STEREO_CAMERA_SUPPORT), yes)

################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/common/vsdof/vsdof_common.mk
#-----------------------------------------------------------
LOCAL_SRC_FILES += stereo_size_provider.cpp

# For __getBufferSize
ifneq ($(wildcard $(LOCAL_PATH)/buffer-size/$(PLATFORM)),)
	BUFFER_SIZE_SRC := buffer-size/$(PLATFORM)/buffer_size.cpp
else ifdef STEREO_ISP_FOLDER
	ifneq ($(wildcard $(LOCAL_PATH)/buffer-size/$(STEREO_ISP_FOLDER)),)
		BUFFER_SIZE_SRC := buffer-size/$(STEREO_ISP_FOLDER)/buffer_size.cpp
	endif
endif
BUFFER_SIZE_SRC ?= buffer-size/default/buffer_size.cpp
LOCAL_SRC_FILES += $(BUFFER_SIZE_SRC)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += \
    $(VSDOF_COMMON_INC) \
    $(LOCAL_PATH)/pass2 \


ifneq ($(wildcard $(LOCAL_PATH)/size-config/$(PLATFORM)),)
	SIZE_CONFIG_FOLDER := $(PLATFORM)
	LOCAL_CFLAGS += -I$(LOCAL_PATH)/size-config/$(PLATFORM)
else ifdef STEREO_ISP_FOLDER
	ifneq ($(wildcard $(LOCAL_PATH)/buffer-size/$(STEREO_ISP_FOLDER)),)
		SIZE_CONFIG_FOLDER := $(STEREO_ISP_FOLDER)
	endif
endif
SIZE_CONFIG_FOLDER ?= default
LOCAL_CFLAGS += -I$(LOCAL_PATH)/size-config/$(SIZE_CONFIG_FOLDER)

ifeq ($(MTKCAM_OPT_VSDOF_HAS_AIDEPTH), 1)
	LOCAL_SHARED_LIBRARIES += libcamalgo.aidepth
endif

ifeq ($(MTKCAM_OPT_VSDOF_HAS_VAIDEPTH), 1)
	LOCAL_SHARED_LIBRARIES += libcamalgo.vaidepth
endif

# Support JSON parse error handling
LOCAL_CPPFLAGS += -fexceptions
#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
#
LOCAL_STATIC_LIBRARIES +=

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
LOCAL_SHARED_LIBRARIES += libdpframework
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog

#-----------------------------------------------------------
LOCAL_MODULE := libfeature.vsdof.hal.size_provider
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
#-----------------------------------------------------------
include $(MTKCAM_STATIC_LIBRARY)


################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))

# endif 	#MTK_CAM_STEREO_CAMERA_SUPPORT
