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
 * @file NodeBufferSizeMgr.cpp
 * @brief Buffer size provider for stereo features
*/

// Standard C header file

// Android system/core header file

// mtkcam custom header file

// mtkcam global header file
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
// Module header file
// Local header file
#include "NodeBufferSizeMgr.h"
#include "../../DepthMapPipe_Common.h"
// Logging header file
#define PIPE_CLASS_TAG "NodeBufferSizeMgr"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace NSCam::NSIoPipe;
using namespace StereoHAL;
/*******************************************************************************
* External Function
********************************************************************************/

/*******************************************************************************
* Enum Define
********************************************************************************/


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::NodeP2ABufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeP2ABufferSize::
NodeP2ABufferSize(
    ENUM_STEREO_SCENARIO scenario,
    sp<DepthMapPipeOption> pOption
)
: P2ABufferSize(scenario)
{
    MY_LOGD("+");
    StereoSizeProvider* pSizePrvder = StereoSizeProvider::getInstance();

    Pass2SizeInfo pass2SizeInfo;
    pSizePrvder->getPass2SizeInfo(PASS2A_P, scenario, pass2SizeInfo);
    mFEAO_AREA_MAIN2      = pass2SizeInfo.areaFEO;

    // frame 1
    pSizePrvder->getPass2SizeInfo(PASS2A, scenario, pass2SizeInfo);
    mFD_IMG_SIZE          = pass2SizeInfo.areaIMG2O;
    mMAIN_IMAGE_SIZE      = pass2SizeInfo.areaWDMA.size;
    // crop info
    if(pOption->mSensorType == v1::Stereo::BAYER_AND_BAYER && !pOption->mbEnableLCE)
        pSizePrvder->getPass2SizeInfo(PASS2A_CROP, scenario, pass2SizeInfo);
    else
        pSizePrvder->getPass2SizeInfo(PASS2A_B_CROP, scenario, pass2SizeInfo);
    mFD_IMG_CROP          = pass2SizeInfo.areaIMG2O;
    mMAIN_IMAGE_CROP      = pass2SizeInfo.areaWDMA;
    mFEB_INPUT_CROP_MAIN1 = pass2SizeInfo.areaWROT;
    // frame 2
    pSizePrvder->getPass2SizeInfo(PASS2A_P_2, scenario, pass2SizeInfo);
    mRECT_IN_SIZE_MAIN2         = pass2SizeInfo.areaWDMA.size;
    mFEBO_AREA_MAIN2            = pass2SizeInfo.areaFEO;
    mRECT_IN_CONTENT_SIZE_MAIN2 = pass2SizeInfo.areaWDMA.size - pass2SizeInfo.areaWDMA.padding;
    // frame 3
    pSizePrvder->getPass2SizeInfo(PASS2A_2, scenario, pass2SizeInfo);
    mRECT_IN_SIZE_MAIN1         = pass2SizeInfo.areaWDMA.size;
    mRECT_IN_CONTENT_SIZE_MAIN1 = pass2SizeInfo.areaWDMA.size - pass2SizeInfo.areaWDMA.padding;
    // frame 4
    pSizePrvder->getPass2SizeInfo(PASS2A_P_3, scenario, pass2SizeInfo);
    mFECO_AREA_MAIN2 = pass2SizeInfo.areaFEO;
    //
    mMYS_SIZE = pSizePrvder->getBufferSize(E_MY_S, scenario);
    //
    // FEInputs
    pSizePrvder->getPass2SizeInfo(PASS2A_2, scenario, pass2SizeInfo);
    mFEB_INPUT_SIZE_MAIN1 = pass2SizeInfo.areaFEO;
    pSizePrvder->getPass2SizeInfo(PASS2A_2, scenario, pass2SizeInfo, false);
    mFEB_INPUT_SIZE_MAIN1_NONSLANT = pass2SizeInfo.areaFEO;
    pSizePrvder->getPass2SizeInfo(PASS2A_3, scenario, pass2SizeInfo);
    mFEC_INPUT_SIZE_MAIN1 = pass2SizeInfo.areaFEO;
    pSizePrvder->getPass2SizeInfo(PASS2A_P_2, scenario, pass2SizeInfo);
    mFEB_INPUT_SIZE_MAIN2 = pass2SizeInfo.areaFEO;
    pSizePrvder->getPass2SizeInfo(PASS2A_P_3, scenario, pass2SizeInfo);
    mFEC_INPUT_SIZE_MAIN2 = pass2SizeInfo.areaFEO;

    MRect rect;
    pSizePrvder->getPass1Size(
                            StereoHAL::eSTEREO_SENSOR_MAIN2,
                            eImgFmt_MTK_YUV_P010,
                            NSImageio::NSIspio::EPortIndex_YUVO,
                            scenario,
                            rect,
                            mMAIN2_YUVO_SIZE,
                            mMAIN2_YUVO_STRIDE);
    pSizePrvder->getPass1Size(
                            StereoHAL::eSTEREO_SENSOR_MAIN1,
                            eImgFmt_MTK_YUV_P010,
                            NSImageio::NSIspio::EPortIndex_YUVO,
                            scenario,
                            rect,
                            mMAIN1_YUVO_SIZE,
                            mMAIN1_YUVO_STRIDE);
    pSizePrvder->getPass1Size(
                            StereoHAL::eSTEREO_SENSOR_MAIN2,
                            eImgFmt_FG_BAYER10,
                            NSImageio::NSIspio::EPortIndex_RRZO,
                            scenario,
                            rect,
                            mMAIN2_RRZO_SIZE,
                            mMAIN2_RRZO_STRIDE);
    pSizePrvder->getPass1Size(
                            StereoHAL::eSTEREO_SENSOR_MAIN1,
                            eImgFmt_Y8,
                            NSImageio::NSIspio::EPortIndex_RSSO_R2,
                            scenario,
                            rect,
                            mMAIN1_RSSOR2_SIZE,
                            mMAIN1_RSSOR2_STRIDE);
    //
    mRECT_OUT1_SIZE = pSizePrvder->getBufferSize(E_RECT_IN_M, scenario);
    debug();
    MY_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::N3DBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeN3DBufferSize::
NodeN3DBufferSize(ENUM_STEREO_SCENARIO scenario)
: N3DBufferSize(scenario)
{
    MY_LOGD("+");
    // setup constants
    StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
    mWARP_IMG_SIZE       = pSizeProvder->getBufferSize(E_MV_Y, scenario);
    mWARP_MASK_SIZE      = pSizeProvder->getBufferSize(E_MASK_M_Y, scenario);
    mWARP_MAP_SIZE_MAIN2 = pSizeProvder->getBufferSize(E_WARP_MAP_S, scenario);
    // main2 in_mask align main2 rsso_r2
    MUINT32 junkStride;
    MSize   szMain2RRZO;
    MRect   tgCropRect;
    pSizeProvder->getPass1Size({ StereoHAL::eSTEREO_SENSOR_MAIN2,
                                 eImgFmt_Y8,
                                 NSImageio::NSIspio::EPortIndex_RSSO_R2,
                                 scenario,
                                 1.0f },
                                tgCropRect,
                                szMain2RRZO,
                                junkStride);
    mWARP_IN_MASK_MAIN2  = szMain2RRZO;
    // needs rotate if vertical module
    const int __moduleRot = StereoSettingProvider::getModuleRotation();
    if (__moduleRot == 90 || __moduleRot == 270) {
        std::swap(mWARP_IN_MASK_MAIN2.w, mWARP_IN_MASK_MAIN2.h);
    }
    debug();
    MY_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::NodeDVSBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeDVSBufferSize::
NodeDVSBufferSize(ENUM_STEREO_SCENARIO scenario)
: DVSBufferSize(scenario)
{
    MY_LOGD("+");
    // setup constans
    StereoSizeProvider* pSizePrvder = StereoSizeProvider::getInstance();
    mDV_LR_SIZE        = pSizePrvder->getBufferSize(E_DV_LR, scenario);
    mCFM_SIZE          = pSizePrvder->getBufferSize(E_CFM_M, scenario);
    mNOC_SIZE          = pSizePrvder->getBufferSize(E_NOC  , scenario);
    mCFM_NONSLANT_SIZE = pSizePrvder->getBufferSize(E_CFM_M, scenario, false);
    mNOC_NONSLANT_SIZE = pSizePrvder->getBufferSize(E_NOC  , scenario, false);
    debug();
    MY_LOGD("+");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::NodeDVPBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeDVPBufferSize::
NodeDVPBufferSize(ENUM_STEREO_SCENARIO scenario)
: DVPBufferSize(scenario)
{
    MY_LOGD("+");
    // setup constans
    StereoSizeProvider* pSizePrvder = StereoSizeProvider::getInstance();
    mASF_CRM_SIZE = pSizePrvder->getBufferSize(E_ASF_CRM, scenario);
    mASF_HF_SIZE  = pSizePrvder->getBufferSize(E_ASF_HF, scenario);
    mASF_RD_SIZE  = pSizePrvder->getBufferSize(E_ASF_RD, scenario);
    mDMW_SIZE     = pSizePrvder->getBufferSize(E_DMW, scenario);
    debug();
    MY_LOGD("+");
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::NodeGFBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeGFBufferSize::
NodeGFBufferSize(ENUM_STEREO_SCENARIO scenario)
: GFBufferSize(scenario)
{
    MY_LOGD("+");
    // setup constants
    StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
    mDEPTHMAP_SIZE = pSizeProvder->getBufferSize(E_DEPTH_MAP, scenario);
    mDMBG_SIZE     = pSizeProvder->getBufferSize(E_DMBG, scenario);
    debug();
    MY_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr::NodeDLDepthBufferSize class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::NodeDLDepthBufferSize::
NodeDLDepthBufferSize(ENUM_STEREO_SCENARIO scenario)
: DLDepthBufferSize(scenario)
{
    MY_LOGD("+");
    // setup constants
    StereoSizeProvider * pSizeProvder = StereoSizeProvider::getInstance();
    mDLDEPTH_SIZE = pSizeProvder->getBufferSize(E_VAIDEPTH_DEPTHMAP, scenario);
    mDLDEPTH_SIZE_NONSLANT = pSizeProvder->getBufferSize(E_VAIDEPTH_DEPTHMAP, scenario, false);
    debug();
    MY_LOGD("-");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  NodeBufferSizeMgr class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NodeBufferSizeMgr::
NodeBufferSizeMgr(sp<DepthMapPipeOption> pOption)
{
    ENUM_STEREO_SCENARIO scen[] = {eSTEREO_SCENARIO_RECORD, eSTEREO_SCENARIO_CAPTURE, eSTEREO_SCENARIO_PREVIEW};

    for(int index=0;index<3;++index)
    {
        mP2ASize.add(scen[index], NodeP2ABufferSize(scen[index], pOption));
        mN3DSize.add(scen[index], NodeN3DBufferSize(scen[index]));
        mDVSSize.add(scen[index], NodeDVSBufferSize(scen[index]));
        mDVPSize.add(scen[index], NodeDVPBufferSize(scen[index]));
        mGFSize.add(scen[index], NodeGFBufferSize(scen[index]));
        mDLDepthSize.add(scen[index], NodeDLDepthBufferSize(scen[index]));
    }
}

NodeBufferSizeMgr::
~NodeBufferSizeMgr()
{
}

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam



