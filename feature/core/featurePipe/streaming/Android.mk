# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2015. All rights reserved.
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
include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/mtkcam_option.mk
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/common/vsdof/vsdof_common.mk
$(info building sfp MTK_PLATFORM=$(MTKCAM_PLATFORM) PLATFORM=$(PLATFORM))

ifeq ($(MTKCAM_LINUX_KERNEL_VERSION),kernel-5.10)
LOCAL_CFLAGS += -DSUPPORT_ION_FD
endif

ifeq ($(strip $(MTK_CAM_NR3D_SUPPORT)),yes)
LOCAL_CFLAGS += -DSUPPORT_3DNR
endif

ifeq (,$(filter $(MTKCAM_TARGET_BOARD_PLATFORM),mt6739 mt8168))
LOCAL_CFLAGS += -DSUPPORT_IMG3O
endif

ifeq ($(MTKCAM_OPT_P2_AND_DEPTH),yes)
LOCAL_CFLAGS += -DSUPPORT_P2GP2_WITH_DEPTH
endif

ifeq ($(MTKCAM_TARGET_BOARD_PLATFORM), mt6739 mt8168)
LOCAL_CFLAGS += -DFORCE_ZOOM_BEFORE_TPI=1
else
LOCAL_CFLAGS += -DFORCE_ZOOM_BEFORE_TPI=0
endif

ifeq ($(MTK_CAM_VSDOF_SUPPORT), yes)
ifneq ($(MTKCAM_IP_BASE),0)
LOCAL_CFLAGS += -DSUPPORT_VSDOF
endif

ifeq ($(MTKCAM_OPT_ISP_VER),40)
LOCAL_CFLAGS+=-DSUPPORT_ISP_VER=40
$(info streaming DepthNode ISP_40 for $(PLATFORM))
else ifeq ($(MTKCAM_OPT_ISP_VER),50)
LOCAL_CFLAGS+=-DSUPPORT_ISP_VER=50
$(info streaming DepthNode ISP_50 for $(PLATFORM))
else ifeq ($(MTKCAM_OPT_ISP_VER),60)
LOCAL_CFLAGS+=-DSUPPORT_ISP_VER=60
$(info streaming DepthNode ISP_60 for $(PLATFORM))
else ifeq ($(MTKCAM_OPT_ISP_VER),65)
LOCAL_CFLAGS+=-DSUPPORT_ISP_VER=65
$(info streaming DepthNode ISP_65 for $(PLATFORM))
else
LOCAL_CFLAGS+=-DSUPPORT_ISP_VER=0
$(info streaming DepthNode ISP_NA for $(PLATFORM))
endif
else
LOCAL_CFLAGS += -DSUPPORT_VSDOF=0
endif

ifeq ($(MTKCAM_OPT_DSDN20),yes)
LOCAL_CFLAGS += -DSUPPORT_VNR_DSDN=1
LOCAL_SHARED_LIBRARIES += libcamalgo.vpudsdn
else
LOCAL_CFLAGS += -DSUPPORT_VNR_DSDN=0
endif

ifeq ($(MTKCAM_OPT_DSDN25),yes)
LOCAL_CFLAGS += -DSUPPORT_DSDN25=1
LOCAL_CFLAGS += -DSUPPORT_MSS=1
else
LOCAL_CFLAGS += -DSUPPORT_DSDN25=0
LOCAL_CFLAGS += -DSUPPORT_MSS=0
endif

ifeq ($(MTK_CAM_EIS_SUPPORT), yes)
LOCAL_CFLAGS += -DSUPPORT_EIS
endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_CFLAGS += -DSUPPORT_WATCHDOG_TIME=40
else
LOCAL_CFLAGS += -DSUPPORT_WATCHDOG_TIME=10
endif
################################################################################
################################################################################
LOCAL_SRC_FILES += StreamingFeature_Common.cpp
LOCAL_SRC_FILES += StreamingFeatureTimer.cpp
LOCAL_SRC_FILES += StreamingFeatureType.cpp
LOCAL_SRC_FILES += StreamingFeatureData.cpp
LOCAL_SRC_FILES += StreamingFeatureNode.cpp
LOCAL_SRC_FILES += StreamingFeatureIO.cpp
LOCAL_SRC_FILES += AdvPQCtrl.cpp
LOCAL_SRC_FILES += SyncCtrl.cpp
LOCAL_SRC_FILES += ImgBufferStore.cpp
LOCAL_SRC_FILES += RootNode.cpp
LOCAL_SRC_FILES += P2AMDPNode.cpp
LOCAL_SRC_FILES += G_P2ANode.cpp
LOCAL_SRC_FILES += G_P2SMNode.cpp
LOCAL_SRC_FILES += P2SWNode.cpp
LOCAL_SRC_FILES += VNRNode.cpp
LOCAL_SRC_FILES += WarpNode.cpp
LOCAL_SRC_FILES += EISNode.cpp
LOCAL_SRC_FILES += HelperNode.cpp
LOCAL_SRC_FILES += VendorMDPNode.cpp
LOCAL_SRC_FILES += TPINode.cpp
LOCAL_SRC_FILES += TPI_AsyncNode.cpp
LOCAL_SRC_FILES += TPI_DispNode.cpp
LOCAL_SRC_FILES += RSCNode.cpp
LOCAL_SRC_FILES += MSSNode.cpp
LOCAL_SRC_FILES += MSFNode.cpp
LOCAL_SRC_FILES += NullNode.cpp
LOCAL_SRC_FILES += XNode.cpp
LOCAL_SRC_FILES += IStreamingFeaturePipe.cpp
LOCAL_SRC_FILES += StreamingFeaturePipe.cpp
LOCAL_SRC_FILES += Dummy_StreamingFeaturePipe.cpp
LOCAL_SRC_FILES += StreamingFeaturePipeUsage.cpp
LOCAL_SRC_FILES += TPIUsage.cpp
LOCAL_SRC_FILES += WarpBase.cpp
LOCAL_SRC_FILES += WarpStream.cpp
LOCAL_SRC_FILES += GPUWarp.cpp
LOCAL_SRC_FILES += MDPWrapper.cpp
LOCAL_SRC_FILES += GPUWarpStream.cpp
LOCAL_SRC_FILES += WPEWarpStream.cpp
LOCAL_SRC_FILES += Dummy_RSCStream.cpp
LOCAL_SRC_FILES += RSCTuningStream.cpp
LOCAL_SRC_FILES += Dummy_WPEWarpStream.cpp
LOCAL_SRC_FILES += VNRHal.cpp
LOCAL_SRC_FILES += EISQControl.cpp
LOCAL_SRC_FILES += FrameControl.cpp
LOCAL_SRC_FILES += OutputControl.cpp
LOCAL_SRC_FILES += DepthNode.cpp
LOCAL_SRC_FILES += BokehNode.cpp
LOCAL_SRC_FILES += TOFNode.cpp
LOCAL_SRC_FILES += SFPIO.cpp
LOCAL_SRC_FILES += DcesoHal.cpp
LOCAL_SRC_FILES += DreHal.cpp
LOCAL_SRC_FILES += TimgoHal.cpp
LOCAL_SRC_FILES += SMVRHal.cpp
LOCAL_SRC_FILES += DsdnHal.cpp
LOCAL_SRC_FILES += p2g/core/P2GDefine.cpp
LOCAL_SRC_FILES += p2g/core/PathEngine.cpp
LOCAL_SRC_FILES += p2g/GImg.cpp
LOCAL_SRC_FILES += p2g/IP2GNode.cpp
LOCAL_SRC_FILES += p2g/SensorContext.cpp
LOCAL_SRC_FILES += p2g/P2GMgr.cpp
LOCAL_SRC_FILES += p2g/P2HWUtil.cpp
LOCAL_SRC_FILES += p2g/P2SWUtil.cpp
LOCAL_SRC_FILES += p2g/PrintWrapper.cpp
LOCAL_SRC_FILES += p2g/SFP_TypeWrapper.cpp
LOCAL_SRC_FILES += NextIOUtil.cpp
# Bokeh
LOCAL_C_INCLUDES += $(VSDOF_COMMON_INC)
LOCAL_SRC_FILES += ColorizedDepthUtil.cpp

#TPI
LOCAL_SRC_FILES += tpi/TPIMgr.cpp
LOCAL_SRC_FILES += tpi/TPIMgr_PluginWrapper.cpp
LOCAL_SRC_FILES += tpi/TPIMgr_PluginPathBuilder.cpp
LOCAL_SRC_FILES += tpi/TPIMgr_Util.cpp

#Tracking Focus
#ifeq ($(MTK_CAM_AITRACKING_SUPPORT),yes)
#LOCAL_CFLAGS += -DSUPPORT_TRACKING=1
#LOCAL_SRC_FILES += TrackingNode.cpp
#else
#LOCAL_CFLAGS += -DSUPPORT_TRACKING=0
#endif
################################################################################
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
################################################################################
LOCAL_C_INCLUDES += $(call include-path-for, camera)
################################################################################
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/core
################################################################################
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/include $(MTK_PATH_SOURCE)/hardware/mtkcam/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/gralloc_extra/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/dpframework/include
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/libs/nativewindow/include
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/libs/arect/include
################################################################################
LOCAL_C_INCLUDES += $(MTKCAM_ALGO_INCLUDE)/libcore
LOCAL_C_INCLUDES += $(MTKCAM_ALGO_INCLUDE)/libutility
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/hardware/libcamera_ext/lib/libwarp
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/hardware/libcamera_ext/vpu/include

LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/libcamera_feature/lib3dnr_lib/include
################################################################################
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/include/common/fb
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/include/common/FaceDetection/FD4.0
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/include/algorithm/$(MTKCAM_PLATFORM_DIR)/libfdft
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/include/algorithm/$(MTKCAM_PLATFORM_DIR)/libvfb
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/include/algorithm/$(MTKCAM_PLATFORM_DIR)/libutility
#LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/include/algorithm/$(MTKCAM_PLATFORM_DIR)/libfov
################################################################################
LOCAL_C_INCLUDES += $(MTK_PATH_COMMON)/hal/inc
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc
LOCAL_C_INCLUDES += $(MTKCAM_PATH_CUSTOM_PLATFORM)/hal/inc/isp_tuning
LOCAL_C_INCLUDES += $(TOP)/$(MTKCAM_PATH_CUSTOM_PLATFORM)
################################################################################
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libladder
LOCAL_SHARED_LIBRARIES += libnativewindow
LOCAL_SHARED_LIBRARIES += libcamalgo.warp
LOCAL_SHARED_LIBRARIES += libcamalgo.fsc
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils libmtkcam_imgbuf
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
LOCAL_SHARED_LIBRARIES += libcam.iopipe
LOCAL_SHARED_LIBRARIES += libmtkcam_rsc
LOCAL_SHARED_LIBRARIES += libdpframework
LOCAL_SHARED_LIBRARIES += libgralloc_extra
LOCAL_SHARED_LIBRARIES += libfeature.face
LOCAL_SHARED_LIBRARIES += libcameracustom
LOCAL_SHARED_LIBRARIES += libfeature_3dnr
LOCAL_SHARED_LIBRARIES += libfeature_fsc
LOCAL_SHARED_LIBRARIES += libmtkcam_tuning_utils
LOCAL_SHARED_LIBRARIES += libcam.feature_utils
LOCAL_SHARED_LIBRARIES += libmtkcam_3rdparty
LOCAL_SHARED_LIBRARIES += libmtkcam_3rdparty.core
LOCAL_SHARED_LIBRARIES += libcamalgo.lmv
LOCAL_SHARED_LIBRARIES += libcamalgo.gyro
LOCAL_SHARED_LIBRARIES += libcamalgo.eis
LOCAL_SHARED_LIBRARIES += libmtkcam_scenariorecorder
LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils
ifeq ($(MTK_CAM_STEREO_CAMERA_SUPPORT), yes)
ifeq ($(MTK_CAM_VSDOF_SUPPORT), yes)
LOCAL_SHARED_LIBRARIES += libfeature.vsdof.hal
endif
endif
ifeq ($(MTKCAM_OPT_DSDN25),yes)
LOCAL_SHARED_LIBRARIES += libmtkcam_mfb
endif
ifeq ($(MTK_CAM_EIS_SUPPORT), yes)
LOCAL_SHARED_LIBRARIES += libfeature_eis
LOCAL_SHARED_LIBRARIES += libcameracustom.eis
endif
ifeq ($(MTK_CAM_VSDOF_SUPPORT), yes)
LOCAL_SHARED_LIBRARIES += libmtkcam.featurepipe.depthmap
LOCAL_SHARED_LIBRARIES += libmtkcam.featurepipe.vsdof_util
endif
LOCAL_SHARED_LIBRARIES += libJpgEncPipe
LOCAL_SHARED_LIBRARIES += libmtkcam_exif
# Dynamic Tuning Use
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
# for dump
LOCAL_SHARED_LIBRARIES += libmtkcam_tuning_utils
LOCAL_SHARED_LIBRARIES += libcamalgo.fdft
ifeq ($(INDEP_STEREO_PROVIDER), true)
LOCAL_SHARED_LIBRARIES += libfeature.stereo.provider
endif
LOCAL_SHARED_LIBRARIES += libhardware
LOCAL_SHARED_LIBRARIES += libion
LOCAL_SHARED_LIBRARIES += libmtkcam_debugutils
LOCAL_SHARED_LIBRARIES += libcam.utils.sensorprovider
################################################################################
LOCAL_STATIC_LIBRARIES += libmtkcam.featurepipe.core
################################################################################
# AIOBJ
#ifeq ($(MTK_CAM_AITRACKING_SUPPORT),yes)
#LOCAL_HEADER_LIBRARIES += OThal_headers
#LOCAL_SHARED_LIBRARIES += libAIOTcore
#LOCAL_SHARED_LIBRARIES += libAIODcore
#LOCAL_SHARED_LIBRARIES += libOThal
#endif
################################################################################
LOCAL_MODULE := libmtkcam.featurepipe.streaming
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTKCAM_SHARED_LIBRARY)
################################################################################
#
################################################################################
#include $(call all-makefiles-under,$(LOCAL_PATH))
#include $(LOCAL_PATH)/vnr_lib/Android.mk
