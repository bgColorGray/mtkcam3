## Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2018. All rights reserved.
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
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
#-----------------------------------------------------------------
include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/mtkcam_option.mk
#-----------------------------------------------------------------
# Default version
CNR_MODULE_VERSION               := default
CNR_MODULE_VERSION_NEW_NVRAM     := default
# MT6785
# {{{
ifeq ($(MTKCAM_TARGET_BOARD_PLATFORM), $(filter $(MTKCAM_TARGET_BOARD_PLATFORM), mt6785))
  CNR_PLATFORM                 := mt6785
  CNR_MODULE_VERSION := 1.0
endif
# }}}

#------------------------------------------------------------------------------
# Define share library.
#------------------------------------------------------------------------------
CNR_SHARED_LIBS := libutils
CNR_SHARED_LIBS += libcutils
# MTKCAM ISP 3A
CNR_SHARED_LIBS += libcam.hal3a.v3
# MTKCAM standard
# SWNR
ifeq ($(MTKCAM_OPT_CNR_MAPPING_MGR),yes)
CNR_SHARED_LIBS += libmtkcam_mapping_mgr
endif
CNR_SHARED_LIBS += libmtkcam_stdutils # MTK cam standard library
CNR_SHARED_LIBS += libmtkcam_imgbuf
CNR_SHARED_LIBS += liblog # log library since Android O
# HIDL HANDL
LOCAL_SHARED_LIBRARIES += libhidlbase
# nvram
ifeq ($(strip $(CNR_MODULE_VERSION)),1.0)
  CNR_SHARED_LIBS += libcameracustom
  CNR_SHARED_LIBS += libmtkcam_modulehelper
  CNR_SHARED_LIBS += libmtkcam_mapping_mgr
endif
CNR_SHARED_LIBS += libcam.iopipe
# For wokring buffer allocate
CNR_SHARED_LIBS += libcam.feature_utils
CNR_SHARED_LIBS += libmtkcam_metadata
# P2 driver
CNR_SHARED_LIBS += libcam.iopipe
# AOSP
#CNR_SHARED_LIBS += libutils_headers libhardware_headers

#include $(call all-makefiles-under, $(CNR_MODULE_VERSION))
include $(LOCAL_PATH)/$(CNR_MODULE_VERSION)/Android.mk
