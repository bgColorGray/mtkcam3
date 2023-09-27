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
LOCAL_MODULE := libmtkcam_system_camera_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(TOP)/system/media/camera/include
include $(MTKCAM_BUILD_HEADER_LIBRARY)
################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
#LOCAL_SRC_FILES += ImageBufferOpsImpl.cpp
LOCAL_SRC_FILES += ImageBufferHeapFactoryImpl.cpp
LOCAL_SRC_FILES += BaseImageBufferHeap.cpp
LOCAL_SRC_FILES += BaseImageBuffer.cpp
LOCAL_SRC_FILES += IonImageBufferHeap.cpp
#LOCAL_SRC_FILES += GrallocImageBufferHeap.cpp
#LOCAL_SRC_FILES += DummyImageBufferHeap.cpp
#LOCAL_SRC_FILES += SecureImageBufferHeap.cpp
LOCAL_SRC_FILES += GraphicImageBufferHeap.cpp
#LOCAL_SRC_FILES += BatchGraphicImageBufferHeap.cpp
LOCAL_SRC_FILES += IonDevice.cpp
LOCAL_SRC_FILES += ConvertImageBuffer.cpp

#-----------------------------------------------------------
ifeq ($(BUILD_MTK_LDVT),true)
MTKCAM_DISABLE_GRALLOC     := '1'
else
MTKCAM_DISABLE_GRALLOC     := '0'
endif

#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += libmtkcam_headers libmtkcam3_headers
LOCAL_HEADER_LIBRARIES += libmtkcam_system_camera_headers
LOCAL_HEADER_LIBRARIES += libsystem_headers
LOCAL_HEADER_LIBRARIES += libgralloc_extra_headers
LOCAL_HEADER_LIBRARIES += libnativewindow_headers

# add for imemdrv due to AOSP ION
ifeq ($(MTKCAM_LINUX_KERNEL_VERSION),kernel-5.10)
LOCAL_HEADER_LIBRARIES += libcam_cam_mem_headers
LOCAL_SHARED_LIBRARIES += libmtkcam_imem
LOCAL_CFLAGS += -DCAMMEM_GKI
endif

#
LOCAL_HEADER_LIBRARIES +=  device_kernel_headers
#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
LOCAL_CFLAGS += -DMTKCAM_DISABLE_GRALLOC="$(MTKCAM_DISABLE_GRALLOC)"
#

#-----------------------------------------------------------
ifeq ($(TARGET_BUILD_VARIANT),user)
LOCAL_CFLAGS += -DMTKCAM_IMG_DEBUG=0
else
LOCAL_CFLAGS += -DMTKCAM_IMG_DEBUG=1
endif
#

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
#
LOCAL_STATIC_LIBRARIES +=

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libmtkcam.debugwrapper
LOCAL_SHARED_LIBRARIES += libnativewindow
#
ifeq ($(strip $(MTK_ION_SUPPORT)), yes)
    LOCAL_SHARED_LIBRARIES += libion libion_mtk
    LOCAL_CFLAGS += -DMTK_ION_SUPPORT
endif
#
ifeq ($(strip $(HAVE_AEE_FEATURE)), yes)
    LOCAL_SHARED_LIBRARIES += libaedv
endif
#
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libmtkcam_debugutils
LOCAL_SHARED_LIBRARIES += libmtkcam_grallocutils
# GrallocImageBufferHeap, GraphicImageBufferHeap
LOCAL_SHARED_LIBRARIES += libgralloc_extra
LOCAL_SHARED_LIBRARIES += libmtkcam_sysutils
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
#
LOCAL_SHARED_LIBRARIES += libmtkcam_imgbuf
#
LOCAL_SHARED_LIBRARIES += libsync
#
LOCAL_HEADER_LIBRARIES +=  libgz_uree_headers
ifeq ($(strip $(MTK_CAM_GENIEZONE_SUPPORT)), yes)
    LOCAL_SHARED_LIBRARIES += libgz_uree
endif # MTK_CAM_GENIEZONE_SUPPORT

#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam_imgbuf_v2
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

#-----------------------------------------------------------
include $(MTKCAM_SHARED_LIBRARY)


################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))

ifneq (,$(filter $(strip $(MTKCAM_TARGET_BOARD_PLATFORM)), mt8195))
MTKCAM_IMGBUF_SUPPORT_AOSP_ION := true
endif


