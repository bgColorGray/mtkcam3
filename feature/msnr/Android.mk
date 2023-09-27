## Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2019. All rights reserved.
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
include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/mtkcam_option.mk
# Default version
MSNR_MODULE_VERSION               := default
MsNR_MODULE_VERSION_NEW_NVRAM     := default

ifeq ($(MTKCAM_OPT_MSNR_VER),1.0)
  MSNR_MODULE_VERSION := 1.0
endif

ifeq ($(strip $(MTK_CAM_SINGLE_NR_SUPPORT)),no)
  MSNR_MODULE_VERSION := default
endif

MSNR_HEADER_LIBRARIES := $(MTKCAM_INCLUDE_HEADER_LIB)
MSNR_HEADER_LIBRARIES += libmtkcam_headers
MSNR_HEADER_LIBRARIES += libmtkcam3_headers
MSNR_HEADER_LIBRARIES += libcameracustom_headers
MSNR_HEADER_LIBRARIES += libutils_headers liblog_headers libhardware_headers
#------------------------------------------------------------------------------
# Define share library.
#------------------------------------------------------------------------------
MSNR_SHARED_LIBS := libutils
MSNR_SHARED_LIBS += libcutils
# MTKCAM standard
MSNR_SHARED_LIBS += libmtkcam_stdutils # MTK cam standard library
MSNR_SHARED_LIBS += libmtkcam_imgbuf
MSNR_SHARED_LIBS += liblog # log library since Android O
MSNR_SHARED_LIBS += libmtkcam_ulog
# nvram
ifeq ($(strip $(MSNR_MODULE_VERSION)),1.0)
  MSNR_SHARED_LIBS += libcameracustom
  MSNR_SHARED_LIBS += libmtkcam_modulehelper
  MSNR_SHARED_LIBS += libmtkcam_mapping_mgr
endif
MSNR_SHARED_LIBS += libcam.iopipe
# For wokring buffer allocate
MSNR_SHARED_LIBS += libcam.feature_utils
MSNR_SHARED_LIBS += libmtkcam_metadata
# AOSP
#MSNR_SHARED_LIBS += libutils_headers libhardware_headers

#include $(call all-makefiles-under, $(MSNR_MODULE_VERSION))
include $(LOCAL_PATH)/$(MSNR_MODULE_VERSION)/Android.mk
