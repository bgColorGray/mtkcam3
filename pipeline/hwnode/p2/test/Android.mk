# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2017. All rights reserved.
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
include $(CLEAR_VARS)
################################################################################
GCOV:=$(strip no)
################################################################################
#
################################################################################
LOCAL_SRC_FILES += ../P2_DispatchProcessor.cpp
LOCAL_SRC_FILES += main.cpp
LOCAL_SRC_FILES += TestP2_Common.cpp
LOCAL_SRC_FILES += TestP2_Mocks.cpp
LOCAL_SRC_FILES += Suite_DispatchProcessor.cpp
################################################################################
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
################################################################################
#LOCAL_C_INCLUDES += $(call include-path-for, camera)
################################################################################
#LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/dpframework/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/core
#LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/core/featurePipe/streaming
#LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/include
################################################################################
LOCAL_C_INCLUDES += ../
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/libs/arect/include
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/libs/nativewindow/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/gralloc_extra/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/include $(MTK_PATH_SOURCE)/hardware/mtkcam/include
LOCAL_C_INCLUDES += $(MTK_PATH_COMMON)/hal/inc
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/$(MTK_CAM_SW_VERSION)
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/aaa
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/aaa/$(MTK_CAM_SW_VERSION)
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/isp_tuning
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/isp_tuning/$(MTK_CAM_SW_VERSION)
################################################################################
LOCAL_C_INCLUDES += $(TOP)/external/gtest/include
################################################################################
LOCAL_WHOLE_STATIC_LIBRARIES += libmtkcam.featurepipe.core
################################################################################
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libstdc++
LOCAL_SHARED_LIBRARIES += libutils
#LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
# LOCAL_SHARED_LIBRARIES += libmtkcam.featurepipe.streaming
LOCAL_SHARED_LIBRARIES += libmtkcam_hwnode
LOCAL_SHARED_LIBRARIES += libcam.feature_utils
LOCAL_SHARED_LIBRARIES += libcam.halsensor
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_imgbuf
LOCAL_SHARED_LIBRARIES += libcamalgo.eis
################################################################################
ifeq ($(strip $(GCOV)),yes)
LOCAL_CFLAGS += --coverage
LOCAL_LDFLAGS += --coverage
endif
################################################################################
LOCAL_MODULE := libmtkcam.p2_node.test
include $(BUILD_NATIVE_TEST)
################################################################################
#
################################################################################