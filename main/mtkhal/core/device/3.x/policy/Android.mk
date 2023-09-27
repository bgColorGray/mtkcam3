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
-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk
#-----------------------------------------------------------
LOCAL_SRC_FILES += DeviceSessionPolicyFactory.cpp
LOCAL_SRC_FILES += DeviceSessionPolicy.cpp
#-----------------------------------------------------------
LOCAL_HEADER_LIBRARIES += libmtkcam_headers libmtkcam3_headers
LOCAL_HEADER_LIBRARIES += libnativewindow_headers
LOCAL_HEADER_LIBRARIES += libnativebase_headers
LOCAL_HEADER_LIBRARIES += libarect_headers
LOCAL_HEADER_LIBRARIES += libmtkcam3_hal_device_tinymw_headers
#
LOCAL_HEADER_LIBRARIES += libcameracustom_headers
#
LOCAL_HEADER_LIBRARIES += libmtkcam_hal_core.device_headers
#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += -DLOG_TAG='"mtkcam-devicesessionpolicy"'
#
LOCAL_CFLAGS += -Werror
#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
#
LOCAL_STATIC_LIBRARIES +=
#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
#
LOCAL_SHARED_LIBRARIES += libmtkcam_imgbuf
LOCAL_SHARED_LIBRARIES += libmtkcam_stdutils
LOCAL_SHARED_LIBRARIES += libmtkcam_metadata
LOCAL_SHARED_LIBRARIES += libmtkcam_metastore
LOCAL_SHARED_LIBRARIES += libmtkcam_modulehelper
#LOCAL_SHARED_LIBRARIES += libmtkcam_hwutils
#LOCAL_SHARED_LIBRARIES += libmtkcam_camcoordinator
#LOCAL_SHARED_LIBRARIES += libmtkcam_sysutils
#LOCAL_SHARED_LIBRARIES += libmtkcam_tuning_utils
#LOCAL_SHARED_LIBRARIES += libmtkcam_faceResultHandler
#
LOCAL_SHARED_LIBRARIES += libmtkcam.debugwrapper
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog
LOCAL_SHARED_LIBRARIES += libmtkcam_hal_core_utils
#
#LOCAL_SHARED_LIBRARIES += libcameracustom
#LOCAL_SHARED_LIBRARIES += libfeature.stereo.provider
#
#LOCAL_SHARED_LIBRARIES += libcam.vhdr
#
#LOCAL_SHARED_LIBRARIES += libcam.feature_utils
#LOCAL_SHARED_LIBRARIES += libmtkcam_sensorcontrol
#LOCAL_SHARED_LIBRARIES += libcam.hal3a.ctrl
# For FeaturePolicy
#LOCAL_SHARED_LIBRARIES += libmtkcam_hal_core_featurepolicy
#-----------------------------------------------------------
LOCAL_MODULE := libmtkcam_hal_core_devicesessionpolicy
LOCAL_MODULE_OWNER := mtk
LOCAL_PROPRIETARY_MODULE := true
#-----------------------------------------------------------
include $(MTKCAM_SHARED_LIBRARY)
################################################################################
#
################################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))
