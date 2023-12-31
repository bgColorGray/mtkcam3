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
include $(CLEAR_VARS)
include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/mtkcam_option.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
################################################################################
#
################################################################################
LOCAL_PATH := $(call my-dir)
PLATFORM := $(call to-lower,$(MTKCAM_MTK_PLATFORM))
################################################################################
#
################################################################################
VER2_5_PLATFORMS := mt6765 mt6768
VER3_PLATFORMS := mt6771 mt6785
DEPTHMAP_VER := default

ifneq (,$(filter $(VER2_5_PLATFORMS),$(PLATFORM)))
  DEPTHMAP_VER := 2.5
else ifneq (,$(filter $(VER3_PLATFORMS),$(PLATFORM)))
  DEPTHMAP_VER := 3.0
else ifeq ($(MTKCAM_OPT_VER4_PLATFORMS),yes)
  DEPTHMAP_VER := 4.0
else ifeq ($(MTKCAM_OPT_VER4_5_PLATFORMS),yes)
  DEPTHMAP_VER := 4.5
endif


$(info  current depthmap version=$(DEPTHMAP_VER)  cur platform=$(PLATFORM))

ifneq ("",$(DEPTHMAP_VER))
    include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/core/featurePipe/vsdof/depthmap/$(DEPTHMAP_VER)/Android.mk
endif
################################################################################
#
################################################################################
