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
# ifeq ($(MTK_CAM_STEREO_CAMERA_SUPPORT), yes)
LOCAL_PATH := $(call my-dir)
ifeq ($(STEREO_TUNINGS_FOLDER),$(LOCAL_PATH))
################################################################################
#
################################################################################
include $(CLEAR_VARS)

-include $(TOP)/$(MTK_PATH_SOURCE)/hardware/mtkcam3/feature/common/vsdof/vsdof_common.mk

#-----------------------------------------------------------
LOCAL_SRC_FILES += stereo_tuning_provider_hw.cpp
LOCAL_SRC_FILES += tunings/hw_mdp_pq_tuning.cpp

LOCAL_SRC_FILES += tunings/hw_dpe_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_dpe_dvs_ctrl_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_dpe_dvs_me_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_dpe_dvs_occ_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_dpe_dvp_ctrl_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_fe_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_fm_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_bokeh_tuning.cpp
LOCAL_SRC_FILES += tunings/hw_isp_fe_tuning.cpp

#-----------------------------------------------------------

# Support JSON parse error handling
LOCAL_CPPFLAGS += -fexceptions

LOCAL_HEADER_LIBRARIES += libstereoprovider_headers

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES +=
#
LOCAL_SHARED_LIBRARIES += libdpframework
LOCAL_SHARED_LIBRARIES += libmtkcam_ulog

#-----------------------------------------------------------

#-----------------------------------------------------------
LOCAL_MODULE := libfeature.vsdof.hal.hw_tunings
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
#-----------------------------------------------------------
include $(MTKCAM_STATIC_LIBRARY)


################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif #ifeq ($(STEREO_TUNINGS_FOLDER),
# endif	#MTK_CAM_STEREO_CAMERA_SUPPORT