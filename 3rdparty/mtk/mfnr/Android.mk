# Copyright Statement:
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

################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
LOCAL_SRC_FILES += MFNRImpl.cpp MFNRShotInfo.cpp

LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_Basic.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_default.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v1_4.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v2_0.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v2_1.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v2_5.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v3_0.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapability_v4_0.cpp
LOCAL_SRC_FILES += MFNRCapability/MFNRCapabilityFactory.cpp

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/include $(MTK_PATH_SOURCE)/hardware/mtkcam/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_COMMON)/hal/inc
LOCAL_C_INCLUDES += $(TOP)/$(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc
#
LOCAL_C_INCLUDES += system/media/camera/include

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
ifeq ($(MTK_CAM_STEREO_CAMERA_SUPPORT), yes)
LOCAL_CFLAGS += -DMTK_CAM_STEREO_CAMERA_SUPPORT=1
endif
#
ifdef MTK_CAM_MFB_MAX_FRAME
LOCAL_CFLAGS += -DMFB_MAX_FRAME=$(MTK_CAM_MFB_MAX_FRAME)
endif
#
ifdef MTK_CAM_MFB_SUPPORT
LOCAL_CFLAGS += -DMFB_SUPPORT=$(MTK_CAM_MFB_SUPPORT)
endif
#-----------------------------------------------------------
LOCAL_STATIC_LIBRARIES +=
#
LOCAL_WHOLE_STATIC_LIBRARIES +=

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libmtkcam_pipeline
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
LOCAL_SHARED_LIBRARIES += libmtkcam_streamutils
LOCAL_SHARED_LIBRARIES += libmtkcam_imgbuf
LOCAL_SHARED_LIBRARIES += libmtkcam_exif
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
#LOCAL_SHARED_LIBRARIES += libmtkcam_3rdparty

#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES := libutils_headers liblog_headers libhardware_headers

#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam.plugin.mfnr
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
include $(MTKCAM_STATIC_LIBRARY)

################################################################################

################################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))
#LOCAL_PATH := $(call my-dir)
#include $(call all-subdir-makefiles)
