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
 * @file DepthQTemplateProvider.cpp
 * @brief QParams template creator for stereo feature on different platforms
 */

// Standard C header file

// Android system/core header file

// mtkcam custom header file
#include <camera_custom_stereo.h>
// mtkcam global header file
#include <mtkcam/def/common.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IHalPostProcPipe.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/drv/iopipe/CamIO/IHalCamIO.h>
#include <mtkcam/feature/lcenr/lcenr.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
// Module header file
#include <n3d_hal.h>
#include <stereo_tuning_provider.h>
// Local header file
#include "DepthQTemplateProvider.h"
// logging
#undef PIPE_CLASS_TAG
#define PIPE_MODULE_TAG "DepthMapPipe"
#define PIPE_CLASS_TAG "DepthQTemplateProvider"
#include <PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH);


/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace StereoHAL;
using namespace NSCam::NSIoPipe::NSPostProc;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DepthQTemplateProvider::
DepthQTemplateProvider(
    sp<DepthMapPipeSetting> pSetting,
    sp<DepthMapPipeOption> pPipeOption,
    DepthMapFlowOption* pFlowOption
)
: miSensorIdx_Main1(pSetting->miSensorIdx_Main1)
, miSensorIdx_Main2(pSetting->miSensorIdx_Main2)
, mpFlowOption(pFlowOption)
, mpPipeOption(pPipeOption)
{
    // Record the frame index of the L/R-Side which is defined by ALGO.
    // L-Side could be main1 or main2 which can be identified by the sensors' locations.
    // Frame 0,3,5 is main1, 1,2,6 is main2.
    if(STEREO_SENSOR_REAR_MAIN_TOP == StereoSettingProvider::getSensorRelativePosition())
    {
        frameIdx_LSide[0] = 3;      // frame index of Left Side(Main1)
        frameIdx_LSide[1] = 5;
        frameIdx_RSide[0] = 2;      // frame index of Right Side(Main2)
        frameIdx_RSide[1] = 4;
    }
    else
    {
        // Main1: R, Main2: L
        frameIdx_LSide[0] = 2;
        frameIdx_LSide[1] = 4;
        frameIdx_RSide[0] = 3;
        frameIdx_RSide[1] = 5;
    }
    // obtain sensor list
    mpIHalSensorList = MAKE_HalSensorList();
    meStereoNormalScenario = mpPipeOption->mStereoScenario;
}

MBOOL
DepthQTemplateProvider::
init(BaseBufferSizeMgr* pSizeMgr)
{
    mpSizeMgr = pSizeMgr;
    MBOOL bRet = prepareTemplateParams();
    bRet &= prepareTemplateParams_Bayer();
    bRet &= prepareTemplateParams_Warping();
    return bRet;
}


DepthQTemplateProvider::
~DepthQTemplateProvider()
{
    MY_LOGD("[Destructor] +");

    mSrzSizeTemplateMap.clear();
    mFETuningBufferMap.clear();
    mFMTuningBufferMap_LtoR.clear();
    mFMTuningBufferMap_RtoL.clear();
    MY_LOGD("[Destructor] -");
}

NSCam::NSIoPipe::PortID
DepthQTemplateProvider::
mapToPortID(DepthMapBufferID buffeID)
{
    switch(buffeID)
    {
        case BID_P2A_IN_RSRAW1:
        case BID_P2A_IN_RSRAW2:
        case BID_P2A_IN_FSRAW1:
        case BID_P2A_IN_FSRAW2:
        case BID_P2A_IN_YUV1:
        case BID_P2A_IN_YUV2:
            return PORT_IMGI;

        case BID_P2A_OUT_FDIMG:
        case BID_P2A_FE1C_INPUT:
        case BID_P2A_FE2C_INPUT:
            return PORT_IMG2O;

        case BID_P2A_OUT_FE1AO:
        case BID_P2A_OUT_FE2AO:
        case BID_P2A_OUT_FE1BO:
        case BID_P2A_OUT_FE2BO:
        case BID_P2A_OUT_FE1CO:
        case BID_P2A_OUT_FE2CO:
            return PORT_FEO;

        case BID_P2A_OUT_RECT_IN1:
        case BID_P2A_OUT_RECT_IN2:
        case BID_P2A_OUT_MV_F:
        case BID_P2A_OUT_MV_F_CAP:
            return PORT_WDMAO;

        case BID_P2A_FE1B_INPUT:
        case BID_P2A_FE1B_INPUT_NONSLANT:
        case BID_P2A_FE2B_INPUT:
        case BID_P2A_OUT_MY_S:
        case BID_P2A_OUT_POSTVIEW:
            return PORT_WROTO;

        case BID_P2A_OUT_FMBO_LR:
        case BID_P2A_OUT_FMBO_RL:
        case BID_P2A_OUT_FMCO_LR:
        case BID_P2A_OUT_FMCO_RL:
            return PORT_FMO;

        case BID_WPE_OUT_SV_Y:
        case BID_WPE_OUT_MASK_S:
        case BID_N3D_OUT_MV_Y:
            return PORT_WPEO;

        #if defined(GTEST) && !defined(GTEST_PROFILE)
        case BID_FE2_HWIN_MAIN1:
        case BID_FE2_HWIN_MAIN2:
        case BID_FE3_HWIN_MAIN1:
        case BID_FE3_HWIN_MAIN2:
            return PORT_IMG3O;
        #endif

        default:
            MY_LOGE("mapToPortID: not exist buffeID=%d", buffeID);
            break;
    }

    return NSCam::NSIoPipe::PortID();

}

MBOOL
DepthQTemplateProvider::
prepareTemplateParams()
{
    MY_LOGD("+");
    //SRZ: crop first, then resize.
    #define CONFIG_SRZINFO_TO_CROPAREA(SrzInfo, StereoArea) \
        SrzInfo.in_w =  StereoArea.size.w;\
        SrzInfo.in_h =  StereoArea.size.h;\
        SrzInfo.crop_w = StereoArea.size.w - StereoArea.padding.w;\
        SrzInfo.crop_h = StereoArea.size.h - StereoArea.padding.h;\
        SrzInfo.crop_x = StereoArea.startPt.x;\
        SrzInfo.crop_y = StereoArea.startPt.y;\
        SrzInfo.crop_floatX = 0;\
        SrzInfo.crop_floatY = 0;\
        SrzInfo.out_w = StereoArea.size.w - StereoArea.padding.w;\
        SrzInfo.out_h = StereoArea.size.h - StereoArea.padding.h;

    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    // FE SRZ template - key is stage num, only stage 1/2 needs FE
    _SRZ_SIZE_INFO_ srzInfo_frame2;
    CONFIG_SRZINFO_TO_CROPAREA(srzInfo_frame2, rP2ASize.mFEBO_AREA_MAIN2);
    mSrzSizeTemplateMap.add(1,  srzInfo_frame2);
    _SRZ_SIZE_INFO_ srzInfo_frame4;
    CONFIG_SRZINFO_TO_CROPAREA(srzInfo_frame4, rP2ASize.mFECO_AREA_MAIN2);
    mSrzSizeTemplateMap.add(2,  srzInfo_frame4);
    // FM tuning template
    for(int iStage=1;iStage<=2;++iStage)
    {
        NSCam::NSIoPipe::FMInfo fmInfo_LR;
        // setup
        setupFMTuningInfo(fmInfo_LR, iStage, MTRUE);
        mFMTuningBufferMap_LtoR.add(iStage, fmInfo_LR);

        NSCam::NSIoPipe::FMInfo fmInfo_RL;
        // setup
        setupFMTuningInfo(fmInfo_RL, iStage, MFALSE);
        mFMTuningBufferMap_RtoL.add(iStage, fmInfo_RL);
    }
    // prepare FE tuning buffer
    for (int iStage=1;iStage<=2;++iStage)
    {
        // setup
        MUINT32 iBlockSize = StereoSettingProvider::fefmBlockSize(iStage);
        NSCam::NSIoPipe::FEInfo feInfo;
        StereoTuningProvider::getFETuningInfo(feInfo, iBlockSize);
        MY_LOGD("FEInfo.mFEMODE=%d", feInfo.mFEMODE);
        mFETuningBufferMap.add(iStage, feInfo);
    }

    // prepare QParams template
    int eRot = StereoSettingProvider::getModuleRotation();
    MINT32 iModuleTrans = remapTransform(eRot);

    MBOOL bRet = MTRUE;
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        bRet &= prepareQParamsTemplate(MFALSE, eSTATE_NORMAL, iModuleTrans, mQParam_NORMAL);
        bRet &= prepareQParamsTemplate(MTRUE,  eSTATE_NORMAL, iModuleTrans, mQParam_NORMAL_NOFEFM);
        bRet &= prepareQParamsTemplate(MFALSE, eSTATE_STANDALONE, iModuleTrans, mQParam_STANDALONE);
    }
    else if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2YUVO_2RSSOR2)
    {
        VSDOF_LOGD("No need prepare QParamTemplate");
    }

    if(!bRet)
    {
        MY_LOGE("prepareQParamsTemplate failed!!!");
    }

    MY_LOGD("-");
    return bRet;
}

MBOOL
DepthQTemplateProvider::
prepareTemplateParams_Bayer()
{
    VSDOF_LOGD2("+");
    // prepare QParams template
    int eRot = StereoSettingProvider::getModuleRotation();
    MINT32 iModuleTrans = remapTransform(eRot);
    MBOOL bRet = MTRUE;
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        bRet &= prepareQParamsTemplate_BAYER_NORMAL(iModuleTrans, mQParam_BAYER_NORMAL);
        bRet &= prepareQParamsTemplate_BAYER_NORMAL_NOFEFM(iModuleTrans, mQParam_BAYER_NORMAL_NOFEFM);
    }
    else
    {
        bRet &= prepareQParamsTemplate_BAYER_NORMAL_ONLYFEFM(iModuleTrans, mQParam_BAYER_NORMAL_ONLYFEFM);
    }
    if(!bRet)
    {
        MY_LOGE("prepareQParamsTemplate failed!!!");
        return MFALSE;
    }
    VSDOF_LOGD2("-");
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
prepareTemplateParams_Warping()
{
    MY_LOGD("+");
    // prepare QParams template
    int eRot = StereoSettingProvider::getModuleRotation();
    MINT32 iModuleTrans = remapTransform(eRot);
    MBOOL bRet = MTRUE;
    bRet &= prepareQParamsTemplate_NORMAL_SLANT(iModuleTrans, mQParam_NORMAL_SLANT);
    bRet &= prepareQParamsTemplate_WARP_NORMAL(mQParam_WARP_NORMAL);
    MY_LOGD("-");
    return bRet;
}

MVOID
DepthQTemplateProvider::
setupFMTuningInfo(NSCam::NSIoPipe::FMInfo& fmInfo, MUINT iStageNum, MBOOL isLtoR)
{
    MY_LOGD("+");

    MSize szFEBufSize = (iStageNum == 1) ? mpSizeMgr->getP2A(meStereoNormalScenario).mFEB_INPUT_SIZE_MAIN1.size :
                                        mpSizeMgr->getP2A(meStereoNormalScenario).mFEC_INPUT_SIZE_MAIN1;

    ENUM_FM_DIRECTION eDir = (isLtoR) ? E_FM_L_TO_R : E_FM_R_TO_L;
    // query tuning parameter
    StereoTuningProvider::getFMTuningInfo(eDir, fmInfo);
    MUINT32 iBlockSize =  StereoSettingProvider::fefmBlockSize(iStageNum);
    // coverity
    if(iBlockSize == 0)
        iBlockSize = 1;
    // set width/height
    fmInfo.mFMWIDTH = szFEBufSize.w/iBlockSize;
    fmInfo.mFMHEIGHT = szFEBufSize.h/iBlockSize;

    MY_LOGD("-");
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_NORMAL(
    MBOOL bIsSkipFEFM,
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    MY_LOGD("+ bIsSkipFEFM=%d iModuleTrans=%d", bIsSkipFEFM, iModuleTrans);
    MUINT iFrameNum = 0;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    MSize szTBD(0, 0);
    MBOOL bSuccess = MTRUE;

    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        if(bIsSkipFEFM)
        {
            bSuccess &= prepareQParam_NORMAL_NOFEFM_frame0(iModuleTrans, rQParam);
        }
        else
        {
            bSuccess &= prepareQParamStage1_Main1(iFrameNum++, iModuleTrans, rQParam);
        }
    }
    else
    {
        MY_LOGE("Not support image format, check flow option");
    }

    MY_LOGD("- success=%d", bSuccess);
    return bSuccess;
}

MVOID
DepthQTemplateProvider::
_prepareLCETemplate(QParamTemplateGenerator& generator)
{
    generator.addInput(PORT_LCEI).
            addInput(PORT_YNR_LCEI);
}

MBOOL
DepthQTemplateProvider::
_fillLCETemplate(
    MUINT iFrameNum,
    sp<DepthMapEffectRequest> pRequest,
    const AAATuningResult& tuningRes,
    QParamTemplateFiller& rQFiller,
    _SRZ_SIZE_INFO_& rSrzInfo
)
{
    if(tuningRes.tuningResult.pLcsBuf == nullptr)
    {
        rQFiller.delInputPort(iFrameNum, PORT_LCEI).
                delInputPort(iFrameNum, PORT_YNR_LCEI);
    }
    else
    {
        if(!setupSRZ4LCENR(pRequest, rSrzInfo))
            return MFALSE;
        auto pBufferHandler = pRequest->getBufferHandler();
        IImageBuffer* pLCSOBuf = pBufferHandler->requestBuffer(eDPETHMAP_PIPE_NODEID_P2A, PBID_IN_LCSO1);
        rQFiller.insertInputBuf(iFrameNum, PORT_LCEI, pLCSOBuf).
                insertInputBuf(iFrameNum, PORT_YNR_LCEI, pLCSOBuf).
                addModuleInfo(iFrameNum, EDipModule_SRZ4, &rSrzInfo);
    }
    return MTRUE;
}


MBOOL
DepthQTemplateProvider::
_fillDefaultWarpMap(
    IImageBuffer* pSrcBuffer,
    IImageBuffer* pWarpMapX,
    IImageBuffer* pWarpMapY
)
{
    if ( pSrcBuffer == nullptr ) {
        MY_LOGE("null source buffer");
        return MFALSE;
    }
    if ( pWarpMapX == nullptr || pWarpMapY == nullptr ) {
        MY_LOGE("null warp map");
        return MFALSE;
    }
    // write identity map: (0,0) (w-1, 0) (0, h-1),  (w-1, h-1)
    int* pBufferVA = (int*) pWarpMapX->getBufVA(0);
    pBufferVA[0] = 0;
    pBufferVA[1] = (pSrcBuffer->getImgSize().w-1)<<5;
    pBufferVA[2] = 0;
    pBufferVA[3] = (pSrcBuffer->getImgSize().w-1)<<5;
    int* pBufferVA2 = (int*) pWarpMapY->getBufVA(0);
    pBufferVA2[0] = 0;
    pBufferVA2[1] = 0;
    pBufferVA2[2] = (pSrcBuffer->getImgSize().h-1)<<5;
    pBufferVA2[3] = (pSrcBuffer->getImgSize().h-1)<<5;
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
setupSRZ4LCENR(
    sp<DepthMapEffectRequest> pRequest,
    _SRZ_SIZE_INFO_& rSizeInfo
)
{
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer* pLCSOBuf = pBufferHandler->requestBuffer(eDPETHMAP_PIPE_NODEID_P2A, BID_P2A_IN_LCSO1);
    IMetadata* pInHalMeta = pBufferHandler->requestMetadata(eDPETHMAP_PIPE_NODEID_P2A, BID_META_IN_HAL_MAIN1);
    LCENR_IN_PARAMS in;
    LCENR_OUT_PARAMS out;

    if(!tryGetMetadata<MRect>(pInHalMeta, MTK_P1NODE_BIN_CROP_REGION, in.rrz_crop_in))
    {
        MY_LOGE("Failed to query MTK_P1NODE_BIN_CROP_REGION metadata!");
        return MFALSE;
    }
    if(!tryGetMetadata<MSize>(pInHalMeta, MTK_P1NODE_BIN_SIZE, in.rrz_in))
    {
        MY_LOGE("Failed to query MTK_P1NODE_BIN_SIZE metadata!");
        return MFALSE;
    }
    if(!tryGetMetadata<MSize>(pInHalMeta, MTK_P1NODE_RESIZER_SIZE, in.rrz_out))
    {
        MY_LOGE("Failed to query MTK_HAL_REQUEST_SENSOR_SIZE metadata!");
        return MFALSE;
    }

    // preview use rrzo
    if(pRequest->getRequestAttr().opState == eSTATE_CAPTURE)
    {
        IImageBuffer* pRawBuf = pBufferHandler->requestBuffer(eDPETHMAP_PIPE_NODEID_P2A, BID_P2A_IN_FSRAW1);
        in.resized = MFALSE;
        in.p2_in = pRawBuf->getImgSize();
    }
    else
    {
        IImageBuffer* pRawBuf = pBufferHandler->requestBuffer(eDPETHMAP_PIPE_NODEID_P2A, BID_P2A_IN_RSRAW1);
        in.resized = MTRUE;
        in.p2_in = pRawBuf->getImgSize();
    }
    in.lce_full = pLCSOBuf->getImgSize();
    calculateLCENRConfig(in, out);
    // write the srz info
    rSizeInfo = out.srz4Param;
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage1_Main2(
    int iFrameNum,
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);

    MBOOL bSuccess =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main2, ENormalStreamTag_Normal).
            addInput(PORT_IMGI).
            // enable MDPCRop2
            addCrop(eCROP_WROT, MPoint(0,0), MSize(0,0), rP2ASize.mFEB_INPUT_SIZE_MAIN2, true).
            addOutput(mapToPortID(BID_P2A_FE2B_INPUT), iModuleTrans).
            generate(rQParam);

    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage1_Main1(
    int iFrameNum,
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    //
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_IMGI);
    // prepare LCE
    this->_prepareLCETemplate(generator);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.addInput(PORT_VIPI).
            // IMG2O : FD
            addCrop(eCROP_CRZ, ptOrigin, szTBD, rP2ASize.mFD_IMG_SIZE).
            addOutput(mapToPortID(BID_P2A_OUT_FDIMG)).
            // WDMA : MV_F
            addCrop(eCROP_WDMA, ptOrigin, szTBD, rP2ASize.mMAIN_IMAGE_SIZE).
            addOutput(mapToPortID(BID_P2A_OUT_MV_F), 0, EPortCapbility_Disp).
            // WROT: FE1B input - depth input (apply MDPCrop)
            addCrop(eCROP_WROT, ptOrigin, szTBD, rP2ASize.mFEB_INPUT_SIZE_MAIN1, true).
            addOutput(mapToPortID(BID_P2A_FE1B_INPUT), iModuleTrans).  // do module rotation
            addOutput(PORT_IMG3O); //for 3dnr
    }
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage2_Main2(
    int iFrameNum,
    QParams& rQParam
)
{
    int iStage = 1;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    MVOID* srzInfo = (MVOID*)&mSrzSizeTemplateMap.valueFor(iStage);

    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main2, ENormalStreamTag_VSDoF_FE).
            addInput(PORT_IMGI).
            addCrop(eCROP_CRZ, MPoint(0,0), rP2ASize.mFEB_INPUT_SIZE_MAIN2, rP2ASize.mFEC_INPUT_SIZE_MAIN2).  // IMG2O: FE2CO input
            addOutput(mapToPortID(BID_P2A_FE2C_INPUT)).
            #if defined(GTEST) && !defined(GTEST_PROFILE)
            addOutput(PORT_IMG3O).
            #endif
            addOutput(PORT_FEO).                           // FEO
            addModuleInfo(EDipModule_SRZ1, srzInfo).       // FEO SRZ1 config
            addExtraParam(EPIPE_FE_INFO_CMD, (MVOID*)&mFETuningBufferMap.valueFor(iStage));

    // if P1 output RECT_IN2, no need P2A_OUT_RECT_IN2
    if(mpPipeOption->mInputImg == eDEPTH_INPUT_IMG_2RRZO_2RSSOR2)
    {
        generator.
            addCrop(eCROP_WDMA, MPoint(0,0), rP2ASize.mFEB_INPUT_SIZE_MAIN2, MSize(0,0)).  // WDMA : Rect_in2
            addOutput(mapToPortID(BID_P2A_OUT_RECT_IN2));
    }

    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage2_Main1(
    int iFrameNum,
    QParams& rQParam
)
{
    int iStage = 1;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);

    QParamTemplateGenerator templateGen =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_VSDoF_FE).
        addInput(PORT_IMGI).
        //FE1CO
        addCrop(eCROP_CRZ, MPoint(0,0), rP2ASize.mFEB_INPUT_SIZE_MAIN1, rP2ASize.mFEC_INPUT_SIZE_MAIN1).
        addOutput(mapToPortID(BID_P2A_FE1C_INPUT)).
        #if defined(GTEST) && !defined(GTEST_PROFILE)
        addOutput(PORT_IMG3O).
        #endif
        addOutput(PORT_FEO).                           // FEO
        addExtraParam(EPIPE_FE_INFO_CMD, (MVOID*)&mFETuningBufferMap.valueFor(iStage));

    MBOOL bSuccess = templateGen.generate(rQParam);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage3_Main2(
    int iFrameNum,
    QParams& rQParam
)
{
    int iStage = 2;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    MVOID* srzInfo = (MVOID*)&mSrzSizeTemplateMap.valueFor(iStage);

    MBOOL bSuccess =
    QParamTemplateGenerator(iFrameNum, miSensorIdx_Main2, ENormalStreamTag_VSDoF_FE).
        addInput(PORT_IMGI).
        #if defined(GTEST) && !defined(GTEST_PROFILE)
        addOutput(PORT_IMG3O).
        #endif
        addOutput(PORT_FEO).                           // FEO
        addModuleInfo(EDipModule_SRZ1, srzInfo).       // FEO SRZ1 config
        addExtraParam(EPIPE_FE_INFO_CMD, (MVOID*)&mFETuningBufferMap.valueFor(iStage)).
        generate(rQParam);

    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage3_Main1(
    int iFrameNum,
    QParams& rQParam
)
{
    int iStage = 2;
    QParamTemplateGenerator templateGen =
    QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_VSDoF_FE).   // frame 5
        addInput(PORT_IMGI).
        #if defined(GTEST) && !defined(GTEST_PROFILE)
        addOutput(PORT_IMG3O).
        #endif
        addOutput(PORT_FEO).   // FEO
        addExtraParam(EPIPE_FE_INFO_CMD, (MVOID*)&mFETuningBufferMap.valueFor(iStage));

    return templateGen.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParam_FM(
    int iFrameNum,
    int iStage,
    MBOOL bIsLtoR,
    QParams& rQParam
)
{
    const NSCam::NSIoPipe::FMInfo* pFMInfo = (bIsLtoR) ?
                                        &mFMTuningBufferMap_LtoR.valueFor(iStage) :
                                        &mFMTuningBufferMap_RtoL.valueFor(iStage);
    MBOOL bSuccess =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_FM).
            addInput(PORT_RFEOI).
            addInput(PORT_LFEOI).
            addOutput(PORT_FMO).
            addExtraParam(EPIPE_FM_INFO_CMD, (MVOID*)pFMInfo).
            generate(rQParam);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
buildQParams_NORMAL(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    CAM_ULOGM_TAGLIFE("P2ANode::buildQParams_NORMAL");
    VSDOF_LOGD2("+, reqID=%d", pRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_NORMAL;
    int iFrameNum = 0;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2ABAYER);
    //
    MBOOL bRet = buildQParamStage1_Main1(iFrameNum++, pRequest, tuningResult, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    return bRet;

}

MBOOL
DepthQTemplateProvider::
buildQParams_BAYER_NORMAL(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    CAM_ULOGM_TAGLIFE("P2ANode::buildQParams_BAYER_NORMAL");
    VSDOF_LOGD2("+, reqID=%d", pRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_BAYER_NORMAL;
    int iFrameNum = 0;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2ABAYER);
    //
    MBOOL bRet = buildQParamStage1_Main2(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage2_Main1(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage2_Main2(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage3_Main1(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage3_Main2(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParam_4FMFrames(iFrameNum, pRequest, tuningResult, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    return bRet;
}

MBOOL
DepthQTemplateProvider::
buildQParams_NORMAL_NOFEFM(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    //P2A preview when skip FEFM-> only output main2 rect_in
    CAM_ULOGM_TAGLIFE("P2ANode::buildQParams_NORMAL");
    VSDOF_LOGD2("+, reqID=%d", pRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_NORMAL_NOFEFM;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2A);

    MBOOL bRet = buildQParam_NORMAL_NOFEFM_frame0(pRequest, tuningResult, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    VSDOF_LOGD2("-");
    return MTRUE;

}

MBOOL
DepthQTemplateProvider::
buildQParamStage1_Main2(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage1_Main2");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    IImageBuffer *frameBuf_RSRAW2 = pBufferHandler->requestBuffer(nodeID, BID_P2A_IN_RSRAW2);
    AAATuningResult tuningRes = tuningResult.tuningRes_main2;
    //
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main2, *pPqParam, EPortIndex_WROTO))
        return MFALSE;
    // output: FE2B input(WROT)
    IImageBuffer* pImgBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_FE2B_INPUT);
    // filler buffers and the raw size crop info
    rQFiller.insertTuningBuf(iFrameNum, tuningRes.tuningBuffer).  // insert tuning data
        insertInputBuf(iFrameNum, PORT_IMGI, frameBuf_RSRAW2). // input: Main2 RSRAW
        // output: FE2B input(WROT, MDPCrop2)
        setCrop(iFrameNum, eCROP_WROT, MPoint(0,0), frameBuf_RSRAW2->getImgSize(),
                    mpSizeMgr->getP2A(meStereoNormalScenario).mFEB_INPUT_SIZE_MAIN2, true).
        insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE2B_INPUT), pImgBuf).
        addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
        setIspProfile(iFrameNum, tuningRes.ispProfile);

    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage1_Main1(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage1_Main1");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input : RSRAW
    IImageBuffer* frameBuf_RSRAW1 = pBufferHandler->requestBuffer(nodeID, BID_P2A_IN_RSRAW1);
    // tuning result
    AAATuningResult tuningRes = tuningResult.tuningRes_main1;
    // output: MV_F
    IImageBuffer* frameBuf_MV_F = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_MV_F);
    // ouput: FE1B_input
    IImageBuffer* pFE1B_Input_Buf = pBufferHandler->requestBuffer(nodeID, BID_P2A_FE1B_INPUT);
    // ouput: IMG3O buffer
    IImageBuffer* pImgBuf_IMG3O = pBufferHandler->requestBuffer(nodeID, BID_P2A_INTERNAL_IMG3O);
    // input: VIPI
    if(mp3DNR_IMG3OBuf.get() == nullptr)
        rQFiller.delInputPort(iFrameNum, PORT_VIPI);
    else
        rQFiller.insertInputBuf(iFrameNum, PORT_VIPI, mp3DNR_IMG3OBuf->mImageBuffer.get());
    // input: IMGI
    rQFiller.insertTuningBuf(iFrameNum, tuningRes.tuningBuffer). // insert tuning data
            insertInputBuf(iFrameNum, PORT_IMGI, frameBuf_RSRAW1); // input: Main1 RSRAW
    // LCE
    if(!this->_fillLCETemplate(iFrameNum, pRequest, tuningRes, rQFiller, mSRZ4Config_LCENR))
        return MFALSE;
    // PQ
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WROTO) ||
        !configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WDMAO))
        return MFALSE;
    // get crop region
    MPoint ptEISStartPt;
    MSize szEISCropSize;
    if(!getEISCropRegion(nodeID, pRequest, ptEISStartPt, szEISCropSize))
        return MFALSE;
    // FD output
    this->_buildQParam_FD_n_FillInternal(nodeID, iFrameNum, pRequest,
                                        ptEISStartPt, szEISCropSize, rQFiller);
    //
    rQFiller.insertOutputBuf(iFrameNum, PORT_WDMAO, frameBuf_MV_F).  // MV_F
            setCrop(iFrameNum, eCROP_WDMA, ptEISStartPt, szEISCropSize, frameBuf_MV_F->getImgSize()).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE1B_INPUT), pFE1B_Input_Buf). //FE1B_input, apply MDPCrop2
            setCrop(iFrameNum, eCROP_WROT, MPoint(0,0), frameBuf_RSRAW1->getImgSize(), pFE1B_Input_Buf->getImgSize(), true).
            insertOutputBuf(iFrameNum, PORT_IMG3O, pImgBuf_IMG3O).
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage2_Main2(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage2_Main2");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input: fe2bBuf_in
    IImageBuffer* fe2bBuf_in = nullptr;
    MBOOL bRet=pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_FE2B_INPUT, fe2bBuf_in);
    // tuning result : main2 FE
    AAATuningResult tuningRes = tuningResult.tuningRes_FE_main2;
    // output: fe2cBuf_in
    IImageBuffer* fe2cBuf_in = pBufferHandler->requestBuffer(nodeID, BID_P2A_FE2C_INPUT);
    // output: FE2BO
    IImageBuffer* pFE2BOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FE2BO);
    if(CC_UNLIKELY(!bRet))
        return MFALSE;
    // PQ
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main2, *pPqParam, EPortIndex_WDMAO))
        return MFALSE;

    rQFiller.insertInputBuf(iFrameNum, PORT_IMGI, fe2bBuf_in).  // fe2bBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE2C_INPUT), fe2cBuf_in). // fe2cBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FE2BO), pFE2BOBuf). // FE2BO
            insertTuningBuf(iFrameNum, tuningRes.tuningBuffer). // tuning data
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    // P1 output P2A_IN_RECT_IN2, no need to generate P2A_OUT_RECT_IN2
    if(!pRequest->isRequestBuffer(BID_P1_OUT_RECT_IN2))
    {
        // Output: Rect_in2
        IImageBuffer* pRectIn2Buf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_RECT_IN2);
        pRectIn2Buf->setExtParam(mpSizeMgr->getP2A(meStereoNormalScenario).mRECT_IN_CONTENT_SIZE_MAIN2);
        // Output: Rect_in2 NEEDs setCrop because the Rect_in size is changed for each scenario
        rQFiller.setCrop(iFrameNum, eCROP_WDMA, MPoint(0,0), fe2bBuf_in->getImgSize(), pRectIn2Buf->getImgSize()).
                insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_RECT_IN2), pRectIn2Buf);
    }
    else
    {
        rQFiller.delOutputPort(iFrameNum, mapToPortID(BID_P2A_OUT_RECT_IN2), eCROP_WDMA);
    }

    #if defined(GTEST) && !defined(GTEST_PROFILE)
    IImageBuffer* pFEInputBuf = pBufferHandler->requestBuffer(nodeID, BID_FE2_HWIN_MAIN2);
    rQFiller.insertOutputBuf(iFrameNum, mapToPortID(BID_FE2_HWIN_MAIN2), pFEInputBuf);
    #endif
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage2_Main1(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage2_Main1");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input: FE1BBuf_in
    IImageBuffer* pFE1BImgBuf = nullptr;
    MBOOL bRet=pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_FE1B_INPUT, pFE1BImgBuf);
    // tuning result : main1 FE
    AAATuningResult tuningRes = tuningResult.tuningRes_FE_main1;
    // output: FE1C_input
    IImageBuffer* pFE1CBuf_in = pBufferHandler->requestBuffer(nodeID, BID_P2A_FE1C_INPUT);
    // output: FE1BO
    IImageBuffer* pFE1BOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FE1BO);
    if(CC_UNLIKELY(!bRet))
        return MFALSE;
    // PQ
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WDMAO))
        return MFALSE;

    rQFiller.insertInputBuf(iFrameNum, PORT_IMGI, pFE1BImgBuf). // FE1BBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE1C_INPUT), pFE1CBuf_in). // FE1C_input
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FE1BO), pFE1BOBuf). // FE1BO
            insertTuningBuf(iFrameNum, tuningRes.tuningBuffer).
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    #if defined(GTEST) && !defined(GTEST_PROFILE)
    IImageBuffer* pFEInputBuf = pBufferHandler->requestBuffer(nodeID, BID_FE2_HWIN_MAIN1);
    rQFiller.insertOutputBuf(iFrameNum, mapToPortID(BID_FE2_HWIN_MAIN1), pFEInputBuf);
    #endif
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage3_Main2(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage3_Main2");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input: fe2coBuf_in
    IImageBuffer* pFE2CBuf_in = nullptr;
    MBOOL bRet=pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_FE2C_INPUT, pFE2CBuf_in);
    // tuning result : main2 FE
    AAATuningResult tuningRes = tuningResult.tuningRes_FE_main2;
    // output: FE2CO
    IImageBuffer* pFE2COBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FE2CO);
    if(CC_UNLIKELY(!bRet))
        return MFALSE;
    //PQ
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main2, *pPqParam, EPortIndex_WDMAO))
        return MFALSE;

    rQFiller.insertInputBuf(iFrameNum, PORT_IMGI, pFE2CBuf_in). // FE2CBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FE2CO), pFE2COBuf). // FE2COBuf
            insertTuningBuf(iFrameNum, tuningRes.tuningBuffer).
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    #if defined(GTEST) && !defined(GTEST_PROFILE)
    IImageBuffer* pFEInputBuf = pBufferHandler->requestBuffer(nodeID, BID_FE3_HWIN_MAIN2);
    rQFiller.insertOutputBuf(iFrameNum, mapToPortID(BID_FE3_HWIN_MAIN2), pFEInputBuf);
    #endif
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage3_Main1(
    int iFrameNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage3_Main1");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();

    // input: fe1coBuf_in
    IImageBuffer* pFE1CBuf_in = nullptr;
    MBOOL bRet=pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_FE1C_INPUT, pFE1CBuf_in);
    // tuning result : main1 FE
    AAATuningResult tuningRes = tuningResult.tuningRes_FE_main1;
    // output: FE1CO
    IImageBuffer* pFE1COBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FE1CO);
    if(CC_UNLIKELY(!bRet))
        return MFALSE;
    //
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WDMAO))
        return MFALSE;

    rQFiller.insertInputBuf(iFrameNum, PORT_IMGI, pFE1CBuf_in). // FE2CBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FE1CO), pFE1COBuf). // FE1CO
            insertTuningBuf(iFrameNum, tuningRes.tuningBuffer).
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).
            setIspProfile(iFrameNum, tuningRes.ispProfile);


    #if defined(GTEST) && !defined(GTEST_PROFILE)
    IImageBuffer* pFEInputBuf = pBufferHandler->requestBuffer(nodeID, BID_FE3_HWIN_MAIN1);
    rQFiller.insertOutputBuf(iFrameNum, mapToPortID(BID_FE3_HWIN_MAIN1), pFEInputBuf);
    #endif
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParam_4FMFrames(
    int iFNum,
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParam_4FMFrames");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2ABAYER;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // tuning result : FM
    AAATuningResult tuningRes = tuningResult.tuningRes_FM;

    // prepare FEO buffers, frame 2~5 has FE output
    IImageBuffer* feoBuf[6] = {NULL};
    pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_OUT_FE2BO, feoBuf[2]);
    pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_OUT_FE1BO, feoBuf[3]);
    pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_OUT_FE2CO, feoBuf[4]);
    pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_OUT_FE1CO, feoBuf[5]);

    //--> frame iFNum: FM - L to R
    int iFrameNum = iFNum;
    // output: FMBO_LR
    IImageBuffer* pFMBOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FMBO_LR);
    MVOID* pTuningBuf = pBufferHandler->requestWorkingTuningBuf(nodeID, BID_P2A_TUNING);

    rQFiller.insertInputBuf(iFrameNum, PORT_RFEOI, feoBuf[frameIdx_LSide[0]]).
            insertInputBuf(iFrameNum, PORT_LFEOI, feoBuf[frameIdx_RSide[0]]).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FMBO_LR), pFMBOBuf).
            insertTuningBuf(iFrameNum, pTuningBuf).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    //--> frame iFNum+1: FM - R to L
    iFrameNum++;
    // output: FMBO_RL
    pFMBOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FMBO_RL);
    pTuningBuf = pBufferHandler->requestWorkingTuningBuf(nodeID, BID_P2A_TUNING);

    rQFiller.insertInputBuf(iFrameNum, PORT_RFEOI, feoBuf[frameIdx_RSide[0]]).
            insertInputBuf(iFrameNum, PORT_LFEOI, feoBuf[frameIdx_LSide[0]]).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FMBO_RL), pFMBOBuf).
            insertTuningBuf(iFrameNum, pTuningBuf).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    //--> frame iFNum+2: FM - L to R
    iFrameNum++;
    // output: FMCO_LR
    pFMBOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FMCO_LR);
    pTuningBuf = pBufferHandler->requestWorkingTuningBuf(nodeID, BID_P2A_TUNING);

    rQFiller.insertInputBuf(iFrameNum, PORT_RFEOI, feoBuf[frameIdx_LSide[1]]).
            insertInputBuf(iFrameNum, PORT_LFEOI, feoBuf[frameIdx_RSide[1]]).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FMCO_LR), pFMBOBuf).
            insertTuningBuf(iFrameNum, pTuningBuf).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    ///--> frame iFNum+3: FM - R to L
    iFrameNum++;
    // output: FMCO_LR
    pFMBOBuf = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_FMCO_RL);
    pTuningBuf = pBufferHandler->requestWorkingTuningBuf(nodeID, BID_P2A_TUNING);

    rQFiller.insertInputBuf(iFrameNum, PORT_RFEOI, feoBuf[frameIdx_RSide[1]]).
            insertInputBuf(iFrameNum, PORT_LFEOI, feoBuf[frameIdx_LSide[1]]).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_OUT_FMCO_RL), pFMBOBuf).
            insertTuningBuf(iFrameNum, pTuningBuf).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage1_Main1_slant(
    int iFrameNum,
    DepthMapRequestPtr& rpRequest,
    EnqueCookieContainer*& rpWpeQparamHolder,
    QParamTemplateFiller& rQFiller)
{
    VSDOF_LOGD2("buildQParamStage1_Main1_slant");
    MBOOL bIsNeedMY_S = mpFlowOption->checkConnected(P2A_TO_DVP_MY_S) ||
                        mpFlowOption->checkConnected(P2A_TO_DLDEPTH_MY_S);
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    // input : MAIN1 YUVO
    IImageBuffer* frameBuf_YUV1 =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_IN_YUV1);
    // warp grid
    IImageBuffer* pWarpMapX =
            pBufferHandler->requestBuffer(nodeID, BID_DEFAULT_WARPMAP_X);
    IImageBuffer* pWarpMapY =
            pBufferHandler->requestBuffer(nodeID, BID_DEFAULT_WARPMAP_Y);
    if ( !_fillDefaultWarpMap(frameBuf_YUV1, pWarpMapX, pWarpMapY) ){
        return MFALSE;
    }
    // ouput: FE1B_input_NonSlant
    IImageBuffer* pFE1B_Input_NonSlant =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_FE1B_INPUT_NONSLANT);
    // input: WPEI
    rQFiller.insertInputBuf(iFrameNum, PORT_WPEI, frameBuf_YUV1).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE1B_INPUT_NONSLANT),
                pFE1B_Input_NonSlant). // FE1B_input_NonSlant
            setCrop(iFrameNum, eCROP_WROT, MPoint(0,0),
                frameBuf_YUV1->getImgSize()/2,
                pFE1B_Input_NonSlant->getImgSize(),
                true);
    // output: MY_S
    if ( bIsNeedMY_S ) {
        const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
        IImageBuffer* pImgBuf_MYS =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_MY_S);
        MSize cropSize_MYS = rP2ASize.mMYS_SIZE.size - rP2ASize.mMYS_SIZE.padding;
        rQFiller.
            insertOutputBuf(iFrameNum, PORT_WDMAO, pImgBuf_MYS).
            setCrop(iFrameNum, eCROP_WDMA, MPoint(0,0),
                frameBuf_YUV1->getImgSize()/2,
                cropSize_MYS);
    } else {
        rQFiller.delOutputPort(iFrameNum, PORT_WDMAO, eCROP_WDMA);
    }
    // warp grid
    rQFiller.addWPEQParam(iFrameNum, frameBuf_YUV1, pWarpMapX, pWarpMapY,
                        NSIoPipe::NSWpe::WPE_MODE_MDP, rpWpeQparamHolder,
                        MTRUE);
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage2_Main1_slant(
    int iFrameNum,
    DepthMapRequestPtr& rpRequest,
    EnqueCookieContainer*& rpWpeQparamHolder,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage2_Main1_slant");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    // input: FE1B_input_NonSlant
    IImageBuffer* pFE1B_Input_NonSlant = nullptr;
    if(!pBufferHandler->getEnqueBuffer(
            nodeID, BID_P2A_FE1B_INPUT_NONSLANT, pFE1B_Input_NonSlant) ) {
        MY_LOGE("BID_P2A_FE1B_INPUT_NONSLANT Missed");
        return MFALSE;
    }
    // warp grid
    IImageBuffer* pWarpMapX =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_WARPMTX_X_MAIN1);
    IImageBuffer* pWarpMapY =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_WARPMTX_Y_MAIN1);
    // output: FE1B_input
    IImageBuffer* pFE1B_Input =
            pBufferHandler->requestBuffer(nodeID, BID_P2A_FE1B_INPUT);
    // input: WPEI
    rQFiller.insertInputBuf(iFrameNum, PORT_WPEI, pFE1B_Input_NonSlant).
            insertOutputBuf(iFrameNum, mapToPortID(BID_P2A_FE1B_INPUT),
                pFE1B_Input). //FE1B_input, apply MDPCrop2
            setCrop(iFrameNum, eCROP_WROT, MPoint(0,0),
                pFE1B_Input_NonSlant->getImgSize(),
                pFE1B_Input->getImgSize(),
                true);
    // warp grid
    rQFiller.addWPEQParam(iFrameNum, pFE1B_Input_NonSlant, pWarpMapX, pWarpMapY,
                        NSIoPipe::NSWpe::WPE_MODE_MDP, rpWpeQparamHolder,
                        MFALSE);
    return MTRUE;
}

// WPENode
MBOOL
DepthQTemplateProvider::
buildQParamStage1_Main2_warp(
    int iFrameNum,
    DepthMapRequestPtr& rpRequest,
    EnqueCookieContainer*& rpWpeQparamHolder,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage1_Main2_warp");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_WPE;
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    IImageBuffer* pImgBuf_Main2RSSOR2 = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_X = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_Y = nullptr;
    // input : Main2 RSSOR2
    // - slanted/normal case: warping engine
    // - vertical case: n3d hal mdp rotate
    const int __moduleRot = StereoSettingProvider::getModuleRotation();
    if (__moduleRot == 90 || __moduleRot == 270) {
        if(!pBufferHandler->getEnqueBuffer(nodeID, BID_P2A_OUT_RECT_IN2, pImgBuf_Main2RSSOR2)) {
            MY_LOGE("BID_P2A_OUT_RECT_IN2 Missed");
            return MFALSE;
        }
    } else {
        pImgBuf_Main2RSSOR2 = pBufferHandler->requestBuffer(nodeID, BID_P1_OUT_RECT_IN2);
    }
    // input : warp map
    if(!pBufferHandler->getEnqueBuffer(nodeID, BID_N3D_OUT_WARPMTX_MAIN2_X, pImgBuf_WarpMtx_Main2_X) ||
       !pBufferHandler->getEnqueBuffer(nodeID, BID_N3D_OUT_WARPMTX_MAIN2_Y, pImgBuf_WarpMtx_Main2_Y))
    {
        MY_LOGE("WARPING_MATRIX Missed");
        return MFALSE;
    }
    // output: warp sub image
    IImageBuffer* pImgBuf_SV_Y = pBufferHandler->requestBuffer(nodeID, BID_WPE_OUT_SV_Y);

    rQFiller.insertInputBuf(iFrameNum, PORT_WPEI, pImgBuf_Main2RSSOR2). // RectIn2
            insertOutputBuf(iFrameNum, mapToPortID(BID_WPE_OUT_SV_Y), pImgBuf_SV_Y). // SV_Y
            addWPEQParam(iFrameNum, pImgBuf_Main2RSSOR2, pImgBuf_WarpMtx_Main2_X, pImgBuf_WarpMtx_Main2_Y,
                        NSIoPipe::NSWpe::WPE_MODE_WPEO, rpWpeQparamHolder);

    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage2_Main2_warp(
    int iFrameNum,
    DepthMapRequestPtr& rpRequest,
    EnqueCookieContainer*& rpWpeQparamHolder,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage2_Main2_warp");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_WPE;
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    // input : sub rectified image
    IImageBuffer* pImgBuf_DefaultMask2 = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_X = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_Y = nullptr;
    //
    pImgBuf_DefaultMask2 = pBufferHandler->requestBuffer(nodeID, BID_WPE_IN_MASK_S);
    if ( !pImgBuf_DefaultMask2 ) {
        MY_LOGE("failed to request buffer pImgBuf_DefaultMask2");
        return MFALSE;
    }
    if(!pBufferHandler->getEnqueBuffer(nodeID, BID_N3D_OUT_WARPMTX_MAIN2_X, pImgBuf_WarpMtx_Main2_X) ||
       !pBufferHandler->getEnqueBuffer(nodeID, BID_N3D_OUT_WARPMTX_MAIN2_Y, pImgBuf_WarpMtx_Main2_Y))
    {
        MY_LOGE("WARPING_MATRIX Missed");
        return MFALSE;
    }
    N3D_HAL::refineMask(mpPipeOption->mStereoScenario, pImgBuf_DefaultMask2, true);
        pImgBuf_DefaultMask2->syncCache(eCACHECTRL_FLUSH);
    // output: sub mask image
    IImageBuffer* pImgBuf_MASK_S = pBufferHandler->requestBuffer(nodeID, BID_WPE_OUT_MASK_S);

    rQFiller.insertInputBuf(iFrameNum, PORT_WPEI, pImgBuf_DefaultMask2). // FE2CBuf_in
            insertOutputBuf(iFrameNum, mapToPortID(BID_WPE_OUT_MASK_S), pImgBuf_MASK_S). // FE1CO
            addWPEQParam(iFrameNum, pImgBuf_DefaultMask2, pImgBuf_WarpMtx_Main2_X, pImgBuf_WarpMtx_Main2_Y,
                        NSIoPipe::NSWpe::WPE_MODE_WPEO, rpWpeQparamHolder);

    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
buildQParamStage3_Main1_warp_slant(
    int iFrameNum,
    DepthMapRequestPtr& rpRequest,
    EnqueCookieContainer*& rpWpeQparamHolder,
    QParamTemplateFiller& rQFiller
)
{
    VSDOF_LOGD2("buildQParamStage3_Main1_warp_slant");
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_WPE;
    sp<BaseBufferHandler> pBufferHandler = rpRequest->getBufferHandler();
    // input : sub rectified image
    IImageBuffer* pImgBuf_Rsso_Main2 = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_X = nullptr;
    IImageBuffer* pImgBuf_WarpMtx_Main2_Y = nullptr;
    //
    pImgBuf_Rsso_Main2 = pBufferHandler->requestBuffer(nodeID, BID_P1_OUT_RECT_IN2);
    //
    return MTRUE;
}


MBOOL
DepthQTemplateProvider::
prepareQParamStage1_Main1_slant(
    int iFrameNum,
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    //
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_WPEI);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.
            // WROT: FE1B input - depth input (apply MDPCrop)
            addCrop(eCROP_WROT, ptOrigin, szTBD, rP2ASize.mFEB_INPUT_SIZE_MAIN1_NONSLANT.size, true).
            addOutput(mapToPortID(BID_P2A_FE1B_INPUT_NONSLANT)).
            // MY_S
            addCrop(eCROP_WDMA, ptOrigin, szTBD, rP2ASize.mMYS_SIZE.size - rP2ASize.mMYS_SIZE.padding).
            addOutput(PORT_WDMAO);
    }
    return generator.generate(rQParam);
    return MTRUE;
}


MBOOL
DepthQTemplateProvider::
prepareQParamStage2_Main1_slant(
    int iFrameNum,
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    //
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_WPEI);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.
            // WROT: FE1B input - depth input (apply MDPCrop)
            addCrop(eCROP_WROT, ptOrigin, szTBD, rP2ASize.mFEB_INPUT_SIZE_MAIN1, true).
            addOutput(mapToPortID(BID_P2A_FE1B_INPUT), iModuleTrans);  // do module rotation
    }
    return generator.generate(rQParam);
    return MTRUE;
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage1_Main2_warp(
    int iFrameNum,
    QParams& rQParam
)
{
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main2, ENormalStreamTag_Normal).
        addInput(PORT_WPEI);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.addOutput(mapToPortID(BID_WPE_OUT_SV_Y));
    }
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage2_Main2_warp(
    int iFrameNum,
    QParams& rQParam
)
{
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main2, ENormalStreamTag_Normal).
        addInput(PORT_WPEI);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.addOutput(mapToPortID(BID_WPE_OUT_MASK_S));
    }
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamStage3_Main1_warp_slant(
    int iFrameNum,
    QParams& rQParam
)
{
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_WPEI);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    {
        generator.addOutput(mapToPortID(BID_N3D_OUT_MV_Y));
    }
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_BAYER_NORMAL(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    VSDOF_LOGD2("+");
    MUINT32 iFrameNum = 0;
    MBOOL bSuccess = prepareQParamStage1_Main2(iFrameNum++, iModuleTrans, rQParam);
    bSuccess &= prepareQParamStage2_Main1(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage2_Main2(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage3_Main1(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage3_Main2(iFrameNum++, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 1, MTRUE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 1, MFALSE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 2, MTRUE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 2, MFALSE, rQParam);
    VSDOF_LOGD2("- : res=%d", bSuccess);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_BAYER_NORMAL_NOFEFM(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    VSDOF_LOGD2("+");
    MBOOL bSuccess = prepareQParam_NORMAL_NOFEFM_frame0(iModuleTrans, rQParam);
    VSDOF_LOGD2("- : res=%d", bSuccess);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_BAYER_NORMAL_ONLYFEFM(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    VSDOF_LOGD2("+");
    MUINT32 iFrameNum = 0;
    MBOOL bSuccess = MTRUE;
    bSuccess &= prepareQParamStage2_Main2(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage2_Main1(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage3_Main2(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage3_Main1(iFrameNum++, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 1, MTRUE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 1, MFALSE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 2, MTRUE, rQParam);
    bSuccess &= prepareQParam_FM(iFrameNum++, 2, MFALSE, rQParam);
    VSDOF_LOGD2("- : res=%d", bSuccess);
    return bSuccess;
}


MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_NORMAL_SLANT(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    VSDOF_LOGD2("+");
    MBOOL bSuccess = MTRUE;
    if ( StereoSettingProvider::isSlantCameraModule() )
    {
        MUINT32 iFrameNum = 0;
        bSuccess &= prepareQParamStage1_Main1_slant(iFrameNum++, iModuleTrans, rQParam);
        bSuccess &= prepareQParamStage2_Main1_slant(iFrameNum++, iModuleTrans, rQParam);
    }
    VSDOF_LOGD2("- : res=%d", bSuccess);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_WARP_NORMAL(
    QParams& rQParam
)
{
    VSDOF_LOGD2("+");
    MUINT32 iFrameNum = 0;
    MBOOL bSuccess = MTRUE;
    bSuccess &= prepareQParamStage1_Main2_warp(iFrameNum++, rQParam);
    bSuccess &= prepareQParamStage2_Main2_warp(iFrameNum++, rQParam);
    // WPE can not enque more than 2 frames!!! exeucte this function after v4l2
    // if ( StereoSettingProvider::isSlantCameraModule() )
    //     bSuccess &= prepareQParamStage3_Main1_warp_slant(iFrameNum++, rQParam);
    VSDOF_LOGD2("- : res=%d", bSuccess);
    return bSuccess;
}

MBOOL
DepthQTemplateProvider::
prepareQParam_NORMAL_NOFEFM_frame0(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    MUINT iFrameNum = 0;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    //
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_IMGI).
        addInput(PORT_VIPI).
        // IMG2O : FD
        addCrop(eCROP_CRZ, ptOrigin, szTBD, rP2ASize.mFD_IMG_SIZE).
        addOutput(PORT_IMG2O).
        // WDMA : MV_F
        addCrop(eCROP_WDMA, ptOrigin, szTBD, rP2ASize.mMAIN_IMAGE_SIZE).
        addOutput(PORT_WDMAO, 0, EPortCapbility_Disp).
        // WROT : MY_S
        addCrop(eCROP_WROT, ptOrigin, szTBD, rP2ASize.mMYS_SIZE.size - rP2ASize.mMYS_SIZE.padding).
        addOutput(PORT_WROTO, iModuleTrans).
        addOutput(PORT_IMG3O); //for 3dnr
    this->_prepareLCETemplate(generator);
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
prepareQParamsTemplate_STANDALONE(
    MINT32 iModuleTrans,
    QParams& rQParam
)
{
    MUINT iFrameNum = 0;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    // crop information
    MPoint ptOrigin(0,0);
    MSize szTBD(0,0);
    //
    QParamTemplateGenerator generator =
        QParamTemplateGenerator(iFrameNum, miSensorIdx_Main1, ENormalStreamTag_Normal).
        addInput(PORT_IMGI).
        addInput(PORT_VIPI).
        // IMG2O : FD
        addCrop(eCROP_CRZ, ptOrigin, szTBD, rP2ASize.mFD_IMG_SIZE).
        addOutput(PORT_IMG2O).
        // WDMA : MV_F
        addCrop(eCROP_WDMA, ptOrigin, szTBD, rP2ASize.mMAIN_IMAGE_SIZE).
        addOutput(PORT_WDMAO, 0, EPortCapbility_Disp).
        addOutput(PORT_IMG3O);
    // LCE
    this->_prepareLCETemplate(generator);
    return generator.generate(rQParam);
}

MBOOL
DepthQTemplateProvider::
buildQParams_BAYER_NORMAL_NOFEFM(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    // generate Main1
    VSDOF_LOGD2("+");
    // copy template
    rOutParam = mQParam_BAYER_NORMAL_NOFEFM;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2ABAYER);
    //
    MBOOL bRet = buildQParam_NORMAL_NOFEFM_frame0(pRequest, tuningResult, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    VSDOF_LOGD2("- res=%d", bRet);
    return bRet;
}

MBOOL
DepthQTemplateProvider::
buildQParam_NORMAL_NOFEFM_frame0(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParamTemplateFiller& rQFiller
)
{
    MUINT iFrameNum = 0;
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    const P2ABufferSize& rP2ASize = mpSizeMgr->getP2A(meStereoNormalScenario);
    MBOOL bIsNeedMY_S = mpFlowOption->checkConnected(P2A_TO_DVP_MY_S) ||
                        mpFlowOption->checkConnected(P2A_TO_DLDEPTH_MY_S);
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input : RSRAW
    IImageBuffer* frameBuf_RSRAW1 = pBufferHandler->requestBuffer(nodeID, BID_P2A_IN_RSRAW1);
    // tuning result : main1
    AAATuningResult tuningRes = tuningResult.tuningRes_main1;
    // output: MV_F
    IImageBuffer* frameBuf_MV_F = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_MV_F);
    // ouput: IMG3O buffer
    IImageBuffer* pImgBuf_IMG3O = pBufferHandler->requestBuffer(nodeID, BID_P2A_INTERNAL_IMG3O);
    // fill inputs - VIPI
    if(mp3DNR_IMG3OBuf.get() == nullptr)
        rQFiller.delInputPort(iFrameNum, PORT_VIPI);
    else
        rQFiller.insertInputBuf(iFrameNum, PORT_VIPI, mp3DNR_IMG3OBuf->mImageBuffer.get());
    // fill input - IMGI
    rQFiller.insertTuningBuf(iFrameNum, tuningRes.tuningBuffer). // insert tuning data
            insertInputBuf(iFrameNum, PORT_IMGI, frameBuf_RSRAW1); // input: Main1 RSRAW
    // LCE
    if(!this->_fillLCETemplate(iFrameNum, pRequest, tuningRes, rQFiller, mSRZ4Config_LCENR_Bayer))
        return MFALSE;
    // get crop region
    MPoint ptEISStartPt;
    MSize szEISCropSize;
    if(!getEISCropRegion(nodeID, pRequest, ptEISStartPt, szEISCropSize))
        return MFALSE;
    //
    PQParam* pPqParam = (PQParam*)pBufferHandler->requestWorkingTuningBuf(nodeID, BID_PQ_PARAM);
    if (pPqParam == nullptr) {
        MY_LOGE("null tuning buffer");
        return MFALSE;
    }
    if(!configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WROTO) ||
        !configurePQParam(nodeID, iFrameNum, pRequest, miSensorIdx_Main1, *pPqParam, EPortIndex_WDMAO, true))
        return MFALSE;
    // FD output
    this->_buildQParam_FD(iFrameNum, pRequest, ptEISStartPt, szEISCropSize, rQFiller);
    //
    rQFiller.insertOutputBuf(iFrameNum, PORT_WDMAO, frameBuf_MV_F).  // MV_F
            setCrop(iFrameNum, eCROP_WDMA, ptEISStartPt, szEISCropSize, frameBuf_MV_F->getImgSize()).
            addExtraParam(iFrameNum, EPIPE_MDP_PQPARAM_CMD, (void*)pPqParam).   // PQ
            insertOutputBuf(iFrameNum, PORT_IMG3O, pImgBuf_IMG3O).  // N3D
            setIspProfile(iFrameNum, tuningRes.ispProfile); // tuning
    // output: MY_S
    if ( bIsNeedMY_S ) {
        IImageBuffer* pImgBuf_MYS = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_MY_S);
        MSize cropSize_MYS = rP2ASize.mMYS_SIZE.size - rP2ASize.mMYS_SIZE.padding;
        rQFiller.
            insertOutputBuf(iFrameNum, PORT_WROTO, pImgBuf_MYS).
            setCrop(iFrameNum, eCROP_WROT, MPoint(0,0), frameBuf_RSRAW1->getImgSize(), cropSize_MYS);
    }
    else {
        rQFiller.delOutputPort(iFrameNum, PORT_WROTO, eCROP_WROT);
    }
    return MTRUE;
}


MBOOL
DepthQTemplateProvider::
buildQParams_BAYER_NORMAL_ONLYFEFM(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    CAM_ULOGM_TAGLIFE("P2ANode::buildQParams_BAYER_NORMAL_ONLYFEFM");
    VSDOF_LOGD2("+, reqID=%d", pRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_BAYER_NORMAL_ONLYFEFM;
    int iFrameNum = 0;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2ABAYER);
    //
    MBOOL bRet = MTRUE;
    bRet &= buildQParamStage2_Main2(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage2_Main1(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage3_Main2(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParamStage3_Main1(iFrameNum++, pRequest, tuningResult, qParamFiller);
    bRet &= buildQParam_4FMFrames(iFrameNum, pRequest, tuningResult, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    return bRet;
}


MBOOL
DepthQTemplateProvider::
buildQParams_STANDALONE(
    DepthMapRequestPtr& pRequest,
    const Stereo3ATuningRes& tuningResult,
    QParams& rOutParam
)
{
    VSDOF_LOGD2("+");
    // copy template
    rOutParam = mQParam_STANDALONE;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, pRequest, eDPETHMAP_PIPE_NODEID_P2A);
    //
    MUINT iFrameNum = 0;
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
    // input : RSRAW
    IImageBuffer* frameBuf_RSRAW1 = pBufferHandler->requestBuffer(nodeID, BID_P2A_IN_RSRAW1);
    // tuning result : main1
    AAATuningResult tuningRes = tuningResult.tuningRes_main1;
    // output: MV_F
    IImageBuffer* frameBuf_MV_F = pBufferHandler->requestBuffer(nodeID, BID_P2A_OUT_MV_F);
    // ouput: IMG3O buffer
    IImageBuffer* pImgBuf_IMG3O = pBufferHandler->requestBuffer(nodeID, BID_P2A_INTERNAL_IMG3O);

    // fill input - IMGI
    qParamFiller.insertTuningBuf(iFrameNum, tuningRes.tuningBuffer). // insert tuning data
            insertInputBuf(iFrameNum, PORT_IMGI, frameBuf_RSRAW1); // input: Main1 RSRAW
    // fill inputs - VIPI
    if(mp3DNR_IMG3OBuf.get() == nullptr)
        qParamFiller.delInputPort(iFrameNum, PORT_VIPI);
    else
        qParamFiller.insertInputBuf(iFrameNum, PORT_VIPI, mp3DNR_IMG3OBuf->mImageBuffer.get());
    // LCE
    if(!this->_fillLCETemplate(iFrameNum, pRequest, tuningRes, qParamFiller, mSRZ4Config_LCENR_Bayer))
        return MFALSE;
    // get crop region
    MPoint ptEISStartPt;
    MSize szEISCropSize;
    if(!getEISCropRegion(eDPETHMAP_PIPE_NODEID_P2ABAYER, pRequest, ptEISStartPt, szEISCropSize))
        return MFALSE;
    // FD output
    this->_buildQParam_FD(iFrameNum, pRequest, ptEISStartPt, szEISCropSize, qParamFiller);
    //
    qParamFiller.insertOutputBuf(iFrameNum, PORT_WDMAO, frameBuf_MV_F).  // MV_F
            setCrop(iFrameNum, eCROP_WDMA, ptEISStartPt, szEISCropSize, frameBuf_MV_F->getImgSize()).
            insertOutputBuf(iFrameNum, PORT_IMG3O, pImgBuf_IMG3O).
            setIspProfile(iFrameNum, tuningRes.ispProfile);

    MBOOL bRet = qParamFiller.validate();
    VSDOF_LOGD2("- res=%d", bRet);
    return bRet;
}


MBOOL
DepthQTemplateProvider::
buildQParams_NORMAL_SLANT(
    DepthMapRequestPtr& rpRequest,
    QParams& rOutParam,
    EnqueCookieContainer*& rpWpeQparamHolder
)
{
    CAM_ULOGM_TAGLIFE("P2ANode::buildQParams_NORMAL_SLANT");
    VSDOF_LOGD2("+, reqID=%d", rpRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_NORMAL_SLANT;
    int iFrameNum = 0;
    MBOOL bRet = MTRUE;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, rpRequest, eDPETHMAP_PIPE_NODEID_P2A);
    bRet &= buildQParamStage1_Main1_slant(iFrameNum++, rpRequest,
                            rpWpeQparamHolder, qParamFiller);
    bRet &= buildQParamStage2_Main1_slant(iFrameNum++, rpRequest,
                            rpWpeQparamHolder, qParamFiller);
    //
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    VSDOF_LOGD2("- res=%d", bRet);
    return bRet;
}


MBOOL
DepthQTemplateProvider::
buildQParams_WARP_NORMAL(
    DepthMapRequestPtr& rpRequest,
    QParams& rOutParam,
    EnqueCookieContainer*& rpWpeQparamHolder
)
{
    CAM_ULOGM_TAGLIFE("WPENode::buildQParams_WARP_NORMAL");
    VSDOF_LOGD2("+, reqID=%d", rpRequest->getRequestNo());
    // copy template
    rOutParam = mQParam_WARP_NORMAL;
    int iFrameNum = 0;
    // filler
    QParamTemplateFiller qParamFiller(rOutParam, rpRequest, eDPETHMAP_PIPE_NODEID_WPE);
    //
    MBOOL bRet = MTRUE;
    bRet &= buildQParamStage1_Main2_warp(iFrameNum++, rpRequest, rpWpeQparamHolder, qParamFiller);
    bRet &= buildQParamStage2_Main2_warp(iFrameNum++, rpRequest, rpWpeQparamHolder, qParamFiller);
    // [TODO] recover this part after V4L2
    // if ( StereoSettingProvider::isSlantCameraModule() )
    //     bRet &= buildQParamStage3_Main1_warp_slant(iFrameNum++, rpRequest, rpWpeQparamHolder, qParamFiller);
    if(!bRet)
        return MFALSE;

    bRet = qParamFiller.validate();
    VSDOF_LOGD2("- res=%d", bRet);
    return bRet;
}


MBOOL
DepthQTemplateProvider::
queryRemappedEISRegion(
    sp<DepthMapEffectRequest> pRequest,
    MSize szEISDomain,
    MSize szRemappedDomain,
    eis_region& rOutRegion
)
{
    DepthMapPipeNodeID nodeID = eDPETHMAP_PIPE_NODEID_P2A;
    IMetadata* pMeta_InHal = pRequest->getBufferHandler()->requestMetadata(nodeID, BID_META_IN_HAL_MAIN1);
    if(!queryEisRegion(pMeta_InHal, rOutRegion))
    {
        MY_LOGE("Failed to query EIS region!");
        return MFALSE;
    }


    VSDOF_LOGD2("Original query-EIS region: startPt=(%d.%d, %d.%d) Size=(%d, %d)",
            rOutRegion.x_int, rOutRegion.x_float, rOutRegion.y_int, rOutRegion.y_float,
            rOutRegion.s.w, rOutRegion.s.h);

    float ratio_width = szRemappedDomain.w * 1.0 / szEISDomain.w;
    float ratio_height = szRemappedDomain.h * 1.0 / szEISDomain.h;

    rOutRegion.x_int = (MUINT32) ceil(rOutRegion.x_int * ratio_width);
    rOutRegion.x_float = (MUINT32) ceil(rOutRegion.x_int * ratio_width);
    rOutRegion.y_int = (MUINT32) ceil(rOutRegion.y_int * ratio_height);
    rOutRegion.y_float = (MUINT32) ceil(rOutRegion.y_float * ratio_height);
    rOutRegion.s.w = ceil(rOutRegion.s.w * ratio_width);
    rOutRegion.s.h = ceil(rOutRegion.s.h * ratio_height);

    VSDOF_LOGD2("Remapped query-EIS region: startPt=(%d.%d, %d.%d) Size=(%d, %d)",
            rOutRegion.x_int, rOutRegion.x_float, rOutRegion.y_int, rOutRegion.y_float,
            rOutRegion.s.w, rOutRegion.s.h);

    return MTRUE;
}

MINT32
DepthQTemplateProvider::
remapTransformForPostview(int moduleRotation, int jpegOrientation)
{
    //Try to rotate back to original and then rotate to jpegOrientation
    MINT32 postViewRot = (360 - moduleRotation + jpegOrientation)%360;
    MINT32 finalPostViewRot = remapTransform((int)postViewRot);
    VSDOF_LOGD2("Postview rotate: moduleRotation:%d,jpegOrientation:%d, postViewRot:%d iModuleTrans:%d",moduleRotation, jpegOrientation, postViewRot, finalPostViewRot);
    return finalPostViewRot;
}

MVOID
DepthQTemplateProvider::
onHandleP2Done(DepthMapPipeNodeID nodeID, sp<DepthMapEffectRequest> pRequest)
{
    const EffectRequestAttrs& attr = pRequest->getRequestAttr();
    // NR3D exist in P2ANode when need n3d
    if(nodeID == eDPETHMAP_PIPE_NODEID_P2A &&
        (attr.opState == eSTATE_NORMAL || attr.opState == eSTATE_RECORD) &&
        pRequest->isRequestBuffer(BID_P2A_OUT_MV_F))
    {
        sp<BaseBufferHandler> pBufferHandler = pRequest->getBufferHandler();
        if(!pBufferHandler->getEnquedSmartBuffer(nodeID, BID_P2A_INTERNAL_IMG3O, mp3DNR_IMG3OBuf))
        {
            MY_LOGE("Failed to get the BID_P2A_INTERNAL_IMG3O buffer!");
        }
    }
}

MBOOL
DepthQTemplateProvider::
getEISCropRegion(
    DepthMapPipeNodeID nodeID,
    sp<DepthMapEffectRequest> pRequest,
    MPoint& rptCropStart,
    MSize& rszCropSize
)
{
    // check EIS crop region
    if (pRequest->getRequestAttr().isEISOn)
    {
        eis_region region;
        IMetadata* pMeta_InHal = pRequest->getBufferHandler()->requestMetadata(nodeID, BID_META_IN_HAL_MAIN1);
        // set MV_F crop for EIS
        if(queryEisRegion(pMeta_InHal, region))
        {
            rptCropStart = MPoint(region.x_int, region.y_int);
            rszCropSize = region.s;
            return MTRUE;
        }
        else
        {
            MY_LOGE("Query EIS Region Failed! reqID=%d.", pRequest->getRequestNo());
            return MFALSE;
        }
    }
    else
    {
        IImageBuffer* frameBuf_RSRAW1 = pRequest->getBufferHandler()->requestBuffer(nodeID, BID_P2A_IN_RSRAW1);
        rptCropStart = MPoint(0, 0);
        rszCropSize = frameBuf_RSRAW1->getImgSize();
        return MTRUE;
    }
}

IImageBuffer*
DepthQTemplateProvider::
get3DNRVIPIBuffer()
{
    if(mp3DNR_IMG3OBuf == nullptr)
        return nullptr;
    else
        return mp3DNR_IMG3OBuf->mImageBuffer.get();
}

MVOID
DepthQTemplateProvider::
_buildQParam_FD(
    int iFrameNum,
    DepthMapRequestPtr pRequest,
    MPoint ptCropStart,
    MSize szCropSize,
    QParamTemplateFiller& rQFiller
)
{
    IImageBuffer* pFdBuf = nullptr;
    MBOOL bExistFD = pRequest->getRequestImageBuffer({.bufferID=BID_P2A_OUT_FDIMG,
                                                        .ioType=eBUFFER_IOTYPE_OUTPUT}, pFdBuf);
    // no FD-> request internal FD
    if(!bExistFD)
    {
        rQFiller.delOutputPort(iFrameNum, PORT_IMG2O, eCROP_CRZ);
    }
    else
    {
        rQFiller.insertOutputBuf(iFrameNum, PORT_IMG2O, pFdBuf).   // FD output
            setCrop(iFrameNum, eCROP_CRZ, ptCropStart, szCropSize, pFdBuf->getImgSize());
    }
}


MVOID
DepthQTemplateProvider::
_buildQParam_FD_n_FillInternal(
    DepthMapPipeNodeID nodeID,
    int iFrameNum,
    DepthMapRequestPtr pRequest,
    MPoint ptCropStart,
    MSize szCropSize,
    QParamTemplateFiller& rQFiller
)
{
    IImageBuffer* pFdBuf = nullptr;
    MBOOL bExistFD = pRequest->getRequestImageBuffer({.bufferID=BID_P2A_OUT_FDIMG,
                                                        .ioType=eBUFFER_IOTYPE_OUTPUT}, pFdBuf);
    // no FD-> request internal FD
    if(!bExistFD)
    {
        pFdBuf = pRequest->getBufferHandler()->requestBuffer(nodeID, BID_P2A_INTERNAL_FD);
        VSDOF_LOGD("no FD buffer, reqID=%d", pRequest->getRequestNo());
    }

    // insert buffer
    rQFiller.insertOutputBuf(iFrameNum, PORT_IMG2O, pFdBuf).   // FD output
            setCrop(iFrameNum, eCROP_CRZ, ptCropStart, szCropSize, pFdBuf->getImgSize());
}

MBOOL
DepthQTemplateProvider::prepareQParamsTemplate(MBOOL bIsSkipFEFM,
                        DepthMapPipeOpState state, MINT32 iModuleTrans, QParams& rQParam)
{
    if (mpSizeMgr == nullptr)
        return MFALSE;

    if (state == eSTATE_NORMAL || state == eSTATE_RECORD)
        return prepareQParamsTemplate_NORMAL(bIsSkipFEFM, iModuleTrans, rQParam);
    else
        return prepareQParamsTemplate_STANDALONE(iModuleTrans, rQParam);

}

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam
