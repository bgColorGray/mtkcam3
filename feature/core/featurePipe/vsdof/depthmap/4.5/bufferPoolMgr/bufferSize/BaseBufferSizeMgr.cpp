/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/**
 * @file BaseBufferSizeMgr.cpp
 * @brief base class for buffer size mgr
*/

// Standard C header file

// Android system/core header file

// mtkcam custom header file

// mtkcam global header file

// Module header file

// Local header file
#include "BaseBufferSizeMgr.h"
// Logging header file
#define PIPE_MODULE_TAG "DepthMapPipe"
#define PIPE_CLASS_TAG "BaseBufferSizeMgr"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

char* SCENARIO_TO_NAME(ENUM_STEREO_SCENARIO sce)
{
    switch(sce)
    {
    #define MAKE_NAME_CASE(name) \
    case name: return #name;
    MAKE_NAME_CASE(eSTEREO_SCENARIO_PREVIEW);
    MAKE_NAME_CASE(eSTEREO_SCENARIO_RECORD);
    MAKE_NAME_CASE(eSTEREO_SCENARIO_CAPTURE);
    #undef MAKE_NAME_CASE
    };

    return "UNKONWN";
}

/*******************************************************************************
* Global Define
********************************************************************************/

/*******************************************************************************
* External Function
********************************************************************************/

/*******************************************************************************
* Enum Define
********************************************************************************/


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::P2ABufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::P2ABufferSize::
debug() const
{
    MY_LOGD("P2A debug size: scenario=%s, ", SCENARIO_TO_NAME(mScenario));

    DEBUG_MSIZE(mFD_IMG_SIZE);
    DEBUG_MSIZE(mFEC_INPUT_SIZE_MAIN1);
    DEBUG_MSIZE(mFEC_INPUT_SIZE_MAIN2);
    DEBUG_MSIZE(mRECT_IN_SIZE_MAIN1);
    DEBUG_MSIZE(mRECT_IN_SIZE_MAIN2);
    DEBUG_MSIZE(mMAIN2_YUVO_SIZE);
    DEBUG_MSIZE(mRECT_OUT1_SIZE);
    DEBUG_MSIZE(mRECT_IN_CONTENT_SIZE_MAIN1);
    DEBUG_MSIZE(mRECT_IN_CONTENT_SIZE_MAIN2);
    DEBUG_MSIZE(mMAIN_IMAGE_SIZE);

    DEBUG_AREA(mFEB_INPUT_SIZE_MAIN1);
    DEBUG_AREA(mFEB_INPUT_SIZE_MAIN2);
    DEBUG_AREA(mFEAO_AREA_MAIN2);
    DEBUG_AREA(mFEBO_AREA_MAIN2);
    DEBUG_AREA(mFECO_AREA_MAIN2);
    DEBUG_AREA(mFD_IMG_CROP);
    DEBUG_AREA(mMAIN_IMAGE_CROP);
    DEBUG_AREA(mFEB_INPUT_CROP_MAIN1);
    DEBUG_AREA(mMYS_SIZE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::N3DBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::N3DBufferSize::
debug() const
{
    MY_LOGD("N3D debug size: scenario=%s, ", SCENARIO_TO_NAME(mScenario));

    DEBUG_MSIZE(mWARP_IMG_SIZE);
    DEBUG_MSIZE(mWARP_MASK_SIZE);
    DEBUG_MSIZE(mWARP_MAP_SIZE_MAIN2);
    DEBUG_MSIZE(mWARP_IN_MASK_MAIN2);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::DVSBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::DVSBufferSize::
debug() const
{
    MY_LOGD("DVS debug size: scenario=%s, ", SCENARIO_TO_NAME(mScenario));

    DEBUG_MSIZE(mDV_LR_SIZE);
    DEBUG_MSIZE(mCFM_SIZE);
    DEBUG_MSIZE(mNOC_SIZE);
    DEBUG_MSIZE(mCFM_NONSLANT_SIZE);
    DEBUG_MSIZE(mNOC_NONSLANT_SIZE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::DVPBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::DVPBufferSize::
debug() const
{
    MY_LOGD("DVP debug size: scenario=%s, ", SCENARIO_TO_NAME(mScenario));

    DEBUG_MSIZE(mASF_CRM_SIZE);
    DEBUG_MSIZE(mASF_HF_SIZE);
    DEBUG_MSIZE(mASF_RD_SIZE);
    DEBUG_MSIZE(mDMW_SIZE);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::GFBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::GFBufferSize::
debug() const
{
    MY_LOGD("GFBufferSize size======>\n");
    DEBUG_MSIZE(mDEPTHMAP_SIZE);
    DEBUG_MSIZE(mDMBG_SIZE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  BaseBufferSizeMgr::DLDepthBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MVOID
BaseBufferSizeMgr::DLDepthBufferSize::
debug() const
{
    MY_LOGD("DLDepthBufferSize size======>\n");
    DEBUG_MSIZE(mDLDEPTH_SIZE);
}


}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam



