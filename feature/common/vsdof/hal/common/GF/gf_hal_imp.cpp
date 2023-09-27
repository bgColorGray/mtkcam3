/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#include "gf_hal_imp.h"
#include <stereo_tuning_provider.h>
#include <mtkcam/aaa/IHal3A.h>
#include <mtkcam/aaa/INvBufUtil.h>
#include <../inc/stereo_dp_util.h>
#include <af_param.h>
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VSDOF_HAL);

Mutex __sLock;
bool GF_HAL_IMP::s_wasAFTrigger  = false;
MPoint GF_HAL_IMP::s_lastGFPoint = MPoint(-1, -1);

std::weak_ptr<GF_HAL> __wpPreviewGF;
std::weak_ptr<GF_HAL> __wpCaptureGF;

std::shared_ptr<GF_HAL>
GF_HAL::createInstance(ENUM_STEREO_SCENARIO eScenario, bool outputDepthmap)
{
    Mutex::Autolock lock(__sLock);
    GF_HAL_INIT_DATA initData;
    initData.eScenario      = eScenario;
    initData.outputDepthmap = outputDepthmap;

    std::shared_ptr<GF_HAL> ptr;
    if(eScenario == eSTEREO_SCENARIO_CAPTURE) {
        ptr = __wpCaptureGF.lock();
        if(!ptr) {
            ptr.reset(new GF_HAL_IMP(initData));
            __wpCaptureGF = ptr;
        }

        return ptr;
    } else {
        ptr = __wpPreviewGF.lock();
        if(!ptr) {
            ptr.reset(new GF_HAL_IMP(initData));
            __wpPreviewGF = ptr;
        }

        return ptr;
    }

    return ptr;
}

std::shared_ptr<GF_HAL>
GF_HAL::createInstance(GF_HAL_INIT_DATA &initData)
{
    Mutex::Autolock lock(__sLock);
    std::shared_ptr<GF_HAL> ptr;
    if(initData.eScenario == eSTEREO_SCENARIO_CAPTURE) {
        ptr = __wpCaptureGF.lock();
        if(!ptr) {
            ptr.reset(new GF_HAL_IMP(initData));
            __wpCaptureGF = ptr;
        }

        return ptr;
    } else {
        ptr = __wpPreviewGF.lock();
        if(!ptr) {
            ptr.reset(new GF_HAL_IMP(initData));
            __wpPreviewGF = ptr;
        }

        return ptr;
    }

    return ptr;
}

GF_HAL_IMP::GF_HAL_IMP(GF_HAL_INIT_DATA &initData)
    : __outputDepthmap((initData.eScenario == eSTEREO_SCENARIO_CAPTURE) ? true : initData.outputDepthmap)
    , __eScenario(initData.eScenario)
    , LOG_ENABLED(StereoSettingProvider::isLogEnabled(LOG_PERPERTY))
    , DMBG_SIZE(StereoSizeProvider::getInstance()->getBufferSize(E_DMBG, initData.eScenario))
    , DMBG_IMG_SIZE(DMBG_SIZE.w*DMBG_SIZE.h)
    , __fastLogger(LOG_TAG, LOG_PERPERTY)
{
    __fastLogger.setSingleLineMode(SINGLE_LINE_LOG);
    //We'll remove padding before passing to algo, so we use depthmap size
#if (0==HAS_HW_DPE)
    __confidenceArea = StereoSizeProvider::getInstance()->getBufferSize(E_CFM_H, __eScenario);
#else
    __confidenceArea = StereoSizeProvider::getInstance()->getBufferSize(E_CFM_M, __eScenario);
#endif
    StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, __confidenceArea.contentSize(), !IS_ALLOC_GB, __confidenceMap);

    int32_t main1Idx, main2Idx;
    StereoSettingProvider::getStereoSensorIndex(main1Idx, main2Idx);
    __isAFSupported = StereoSettingProvider::isSensorAF(main1Idx);

    const char *HAL3A_QUERY_NAME = "MTKStereoCamera";
    IHal3A *pHal3A = MAKE_Hal3A(main1Idx, HAL3A_QUERY_NAME);
    if(NULL == pHal3A) {
        MY_LOGE("Cannot get 3A HAL");
    } else {
        pHal3A->send3ACtrl(NS3Av3::E3ACtrl_GetAFDAFTable, (MUINTPTR)&__pAFTable, 0);
        MY_LOGE_IF((NULL == __pAFTable), "Cannot get AF table");
        pHal3A->destroyInstance(HAL3A_QUERY_NAME);
    }

    MY_LOGD("Min P1 Crop Ratio %f", __MIN_P1_CROP_RATIO);
    _initAFWinTransform();
    _initFDDomainTransform();

    __pGfDrv = MTKGF::createInstance(DRV_GF_OBJ_SW);
    CAM_ULOG_ASSERT(NSCam::Utils::ULog::MOD_VSDOF_HAL, __pGfDrv != nullptr, "GF instance is NULL!");

    //Init GF
    ::memset(&__initInfo, 0, sizeof(GFInitInfo));
    ::memset(&__procInfo, 0, sizeof(__procInfo));
    ::memset(&__resultInfo, 0, sizeof(__resultInfo));
    //=== Init sizes ===
    __depthInputArea = StereoSizeProvider::getInstance()->getBufferSize(
                            (__eScenario == eSTEREO_SCENARIO_PREVIEW) ? E_DMW : E_VAIDEPTH_DEPTHMAP,
                            __eScenario, false);

    MSize inputSize = __depthInputArea.contentSize();
    __initInfo.inputWidth  = inputSize.w;
    __initInfo.inputHeight = inputSize.h;

    const bool IS_VERTICAL_MODULE = StereoHAL::isVertical(StereoSettingProvider::getModuleRotation());
    if(!StereoSettingProvider::isMTKDepthmapMode()) {
        if(IS_VERTICAL_MODULE) {
            __initInfo.outputWidth  = DMBG_SIZE.h;
            __initInfo.outputHeight = DMBG_SIZE.w;
        } else {
            __initInfo.outputWidth  = DMBG_SIZE.w;
            __initInfo.outputHeight = DMBG_SIZE.h;
        }
    } else {
        MSize outSize = StereoSizeProvider::getInstance()->getBufferSize(E_DEPTH_MAP, __eScenario);
        if(IS_VERTICAL_MODULE) {
            __initInfo.outputWidth  = outSize.h;
            __initInfo.outputHeight = outSize.w;
        } else {
            __initInfo.outputWidth  = outSize.w;
            __initInfo.outputHeight = outSize.h;
        }
    }

    //=== Init tuning info ===
    __initInfo.pTuningInfo = new GFTuningInfo();
    vector<MINT32> tuningTable;
    vector<MINT32> dispCtrlPoints;
    vector<MINT32> blurGainTable;
    std::vector<std::pair<std::string, int>> tuningParamsList;
    StereoTuningProvider::getGFTuningInfo(__initInfo.pTuningInfo->coreNumber, tuningTable, dispCtrlPoints, blurGainTable, tuningParamsList, __eScenario);
    __initInfo.pTuningInfo->clrTblSize = tuningTable.size();
    __initInfo.pTuningInfo->clrTbl     = NULL;
    if(__initInfo.pTuningInfo->clrTblSize > 0) {
        //GF will copy this array, so we do not need to delete __initInfo.pTuningInfo->clrTbl,
        //since tuningTable is a local variable
        __initInfo.pTuningInfo->clrTbl = &tuningTable[0];
    }
    __initInfo.pTuningInfo->ctrlPointNum   = dispCtrlPoints.size();
    __initInfo.pTuningInfo->dispCtrlPoints = NULL;
    __initInfo.pTuningInfo->blurGainTable  = NULL;
    if(__initInfo.pTuningInfo->ctrlPointNum > 0) {
        __initInfo.pTuningInfo->dispCtrlPoints = &dispCtrlPoints[0];
        __initInfo.pTuningInfo->blurGainTable = &blurGainTable[0];
    }
#ifdef GF_CUSTOM_PARAM
    vector<GFTuningParam> tuningParams;
    for(auto &param : tuningParamsList) {
        tuningParams.push_back({(char *)param.first.c_str(), param.second});
    }

    if(!__isAFSupported) {
        tuningParams.push_back({(char *)"gf.is_ff", 1});
    }

    __initInfo.pTuningInfo->NumOfParam = tuningParams.size();
    __initInfo.pTuningInfo->params = &tuningParams[0];
#endif

    if(!__outputDepthmap) {
        __initInfo.gfMode = GF_MODE_VR;
    } else {
        __initInfo.gfMode = GF_MODE_DPP;
        // if(__eScenario != eSTEREO_SCENARIO_CAPTURE)
        // {
        //     __initInfo.gfMode = GF_MODE_DPP;
        // }
        // else
        // {
        //     switch(CUSTOM_DEPTHMAP_SIZE) {     //Defined in camera_custo__stereo.cpp
        //     case STEREO_DEPTHMAP_1X:
        //     default:
        //         __initInfo.gfMode = GF_MODE_CP;
        //         break;
        //     case STEREO_DEPTHMAP_2X:
        //         __initInfo.gfMode = GF_MODE_CP_2X;
        //         break;
        //     case STEREO_DEPTHMAP_4X:
        //         __initInfo.gfMode = GF_MODE_CP_4X;
        //         break;
        //     }
        // }
    }

    // Init __initInfo.cam
    {
        __initInfo.cam.baseline     = StereoSettingProvider::getStereoBaseline();
        __initInfo.cam.focalLength  = initData.focalLenght;
        __initInfo.cam.fNumber      = initData.aperture;
        __initInfo.cam.sensorWidth  = initData.sensorPhysicalWidth;

        int32_t main1Idx, main2Idx;
        StereoSettingProvider::getStereoSensorIndex(main1Idx, main2Idx);
        IHalSensorList* pSensorList = MAKE_HalSensorList();
        if(NULL == pSensorList)
        {
            MY_LOGE("Cannot get sensor list");
            return;
        }

        IHalSensor* pIHalSensor = pSensorList->createSensor(LOG_TAG, main1Idx);
        int err = 0;
        if(NULL == pIHalSensor) {
            MY_LOGE("Cannot get hal sensor of sensor %d", main1Idx);
            err = 1;
        } else {
            SensorCropWinInfo sensorCropInfo;
            ::memset(&sensorCropInfo, 0, sizeof(sensorCropInfo));
            int32_t main1DevIdx = pSensorList->querySensorDevIdx(main1Idx);
            int main1SensorSenario = StereoSettingProvider::getSensorScenarioMain1();
            err = pIHalSensor->sendCommand(main1DevIdx, SENSOR_CMD_GET_SENSOR_CROP_WIN_INFO,
                                           (MUINTPTR)&main1SensorSenario, (MUINTPTR)&sensorCropInfo, 0);
            pIHalSensor->destroyInstance(LOG_TAG);

            __initInfo.cam.pixelArrayW  = sensorCropInfo.full_w;
            __initInfo.cam.sensorSizeW  = sensorCropInfo.w0_size;
            __initInfo.cam.sensorScaleW = sensorCropInfo.scale_w;
        }

        MUINT32 junkStride;
        MSize   szMain1RRZO;
        MRect   tgCrop;
        StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN1,
                                                           eImgFmt_FG_BAYER10,
                                                           EPortIndex_RRZO,
                                                           __eScenario,
                                                           1.0f
                                                         },
                                                         //below are outputs
                                                         tgCrop,
                                                         szMain1RRZO,
                                                         junkStride);
        __initInfo.cam.rrzUsageW = tgCrop.s.w;

        __initInfo.cam.renderWidth  = StereoSizeProvider::getInstance()->getPreviewSize().w;
    }

    if(__loadConvergence()) {
        MY_LOGD_IF(LOG_ENABLED, "Convergence data loaded");
    }

    __pGfDrv->Init((void *)&__initInfo, NULL);
    //Get working buffer size
    __pGfDrv->FeatureCtrl(GF_FEATURE_GET_WORKBUF_SIZE, NULL, &__initInfo.workingBuffSize);

    //Allocate working buffer and set to GF
    if(__initInfo.workingBuffSize > 0) {
        if(StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_Y8, MSize(__initInfo.workingBuffSize, 1), !IS_ALLOC_GB, __workBufImage)) {
            __initInfo.workingBuffAddr = (MUINT8 *)__workBufImage.get()->getBufVA(0);
            MY_LOGD_IF(LOG_ENABLED, "Alloc %d bytes for GF working buffer", __initInfo.workingBuffSize);
            __pGfDrv->FeatureCtrl(GF_FEATURE_SET_WORKBUF_ADDR, &__initInfo, NULL);
        } else {
            MY_LOGE("Cannot create GF working buffer of size %d", __initInfo.workingBuffSize);
        }
    }

    __logInitData();

    if(initData.eScenario != eSTEREO_SCENARIO_CAPTURE) {
        __GFAFThread = std::thread([this]{__runGFAFThread();});
    }
};

GF_HAL_IMP::~GF_HAL_IMP()
{
    MY_LOGD("%s +", (__eScenario == eSTEREO_SCENARIO_CAPTURE)?"Cap":"PV");

    __wakeupGFAF = true;
    __leaveGFAF = true;
    __GFAFCondVar.notify_all(); //wakeup __GFAFThread and terminate

    if(__GFAFThread.joinable()) {
        MY_LOGD("Wait GFAF thread to stop");
        __GFAFThread.join();
        MY_LOGD("GFAF thread stopped");
    } else {
        MY_LOGD("GFAF thread is not joinable");
    }

    //Delete working buffer
    if(__initInfo.pTuningInfo) {
        delete __initInfo.pTuningInfo;
        __initInfo.pTuningInfo = NULL;
    }

    StereoDpUtil::freeImageBuffer(LOG_TAG, __confidenceMap);
    StereoDpUtil::freeImageBuffer(LOG_TAG, __workBufImage);

    _clearTransformedImages();

    if(__pGfDrv) {
        __pGfDrv->Reset();
        __pGfDrv->destroyInstance(__pGfDrv);
        __pGfDrv = NULL;
    }
    MY_LOGD("%s -", (__eScenario == eSTEREO_SCENARIO_CAPTURE)?"Cap":"PV");
}

bool
GF_HAL_IMP::GFHALRun(GF_HAL_IN_DATA &inData, GF_HAL_OUT_DATA &outData)
{
    AutoProfileUtil profile(LOG_TAG, "GFHALRun");
    Mutex::Autolock lock(__sLock);

    std::unique_lock<std::mutex> lk(__GFAFMutex); //wait last GFAF to finish
    lk.unlock();

    __dumpHint = inData.dumpHint;
    __magicNumber = (inData.magicNumber > 0) ? inData.magicNumber : 0;
    __requestNumber = (inData.requestNumber > 0) ? inData.requestNumber : 0;

    __dumpInitInfo();

    if(NULL == inData.depthMap) {
        if(outData.dmbg) {
            MSize szDMBG = outData.dmbg->getImgSize();
            ::memset((void*)outData.dmbg->getBufVA(0), 0, szDMBG.w*szDMBG.h);
        }

        if(outData.depthMap) {
            MSize szDepthMap = outData.depthMap->getImgSize();
            ::memset((void*)outData.depthMap->getBufVA(0), 0, szDepthMap.w*szDepthMap.h);
        }

        return true;
    }

    if(eSTEREO_SCENARIO_CAPTURE == inData.scenario) {   //Preview will be here, too
        // CPUMask cpuMask(CPUCoreB, 2);
        // AutoCPUAffinity affinity(cpuMask);

        _setGFParams(inData);
        _runGF(outData);
    } else {
        _setGFParams(inData);
        _runGF(outData);
    }

    return true;
}

bool
GF_HAL_IMP::GFAFRun(GF_HAL_IN_DATA &inData)
{
    Mutex::Autolock lock(__sLock);
    if(NULL == inData.nocMap ||
       NULL == inData.pdMap ||
       NULL == inData.confidenceMap) {
        MY_LOGE("Invalid input NOC: %p, PD: %p, Conf: %p");

        return false;
    }

    AutoProfileUtil profile(LOG_TAG, "GFAFRun");
    //TODO: integrate new GF and AF API
    // 1. Fill param to __procInfo.afInfo
    _setGFParams(inData);

    // 2. Notify thread to go
    std::unique_lock<std::mutex> lk(__GFAFMutex); //wait last GFAF to finish if needed
    __wakeupGFAF = true;
    lk.unlock();
    __GFAFCondVar.notify_all(); //run __runGFAFThread

    return true;
}

void
GF_HAL_IMP::_initAFWinTransform()
{
    auto &param = __getAFTransformParam(1.0f);
    MY_LOGD_IF(LOG_ENABLED, "AF Transform: offset(%d, %d), scale(%f, %f)",
               param.offsetX, param.offsetY, param.scaleW, param.scaleH);
}

void
GF_HAL_IMP::_initFDDomainTransform()
{
    auto &param = __getFDTransformParam(1.0f);
    MY_LOGD_IF(LOG_ENABLED, "FD Transform for 1x: offset(%d, %d), scale(%f, %f)",
               param.offsetX, param.offsetY, param.scaleW, param.scaleH);
}

MPoint
GF_HAL_IMP::_AFToGFPoint(const MPoint &ptAF)
{
    auto &param = __getAFTransformParam(__procInfo.zoomRatioP1);
    return MPoint( (ptAF.x -param.offsetX) *param.scaleW ,
                   (ptAF.y -param.offsetY) *param.scaleH );
}

MRect
GF_HAL_IMP::_getAFRect(const size_t AF_INDEX)
{
    DAF_VEC_STRUCT *afVec = &__pAFTable->daf_vec[AF_INDEX];
    MPoint topLeft = _AFToGFPoint(MPoint(afVec->af_win_start_x, afVec->af_win_start_y));
    MPoint bottomRight = _AFToGFPoint(MPoint(afVec->af_win_end_x, afVec->af_win_end_y));

    MRect focusRect = StereoHAL::rotateRect( MRect(topLeft, MSize(bottomRight.x - topLeft.x + 1, bottomRight.y - topLeft.y + 1)),
                                             DMBG_SIZE,
                                             StereoSettingProvider::getModuleRotation() );

    //Handle out-of-range, center point will not change, but size will
    if(focusRect.p.x < 0) {
        focusRect.s.w -= -focusRect.p.x * 2;
        focusRect.p.x = 0;
    }

    if(focusRect.p.y < 0) {
        focusRect.s.h -= -focusRect.p.y * 2;
        focusRect.p.y = 0;
    }

    int offset = focusRect.p.x + focusRect.s.w - DMBG_SIZE.w;
    if(offset > 0) {
        focusRect.s.w -= offset * 2;
        focusRect.p.x += offset;
    }

    offset = focusRect.p.y + focusRect.s.h - DMBG_SIZE.h;
    if(offset > 0) {
        focusRect.s.h -= offset * 2;
        focusRect.p.y += offset;
    }

    MY_LOGD_IF(LOG_ENABLED,
            "AF ROI (%d %d) (%d %d) center (%d, %d), GF (%d %d) (%d %d) center (%d, %d), Focus (%d, %d), %dx%d center (%d, %d)",
            afVec->af_win_start_x, afVec->af_win_start_y, afVec->af_win_end_x, afVec->af_win_end_y,
            (afVec->af_win_start_x+afVec->af_win_end_x)/2, (afVec->af_win_start_y+afVec->af_win_end_y)/2,
            topLeft.x, topLeft.y, bottomRight.x, bottomRight.y,
            (topLeft.x+bottomRight.x)/2, (topLeft.y+bottomRight.y)/2,
            focusRect.p.x, focusRect.p.y, focusRect.s.w, focusRect.s.h,
            focusRect.p.x+focusRect.s.w/2, focusRect.p.y+focusRect.s.h/2);

    return focusRect;
}

MPoint
GF_HAL_IMP::_getTouchPoint(MPoint ptIn)
{
    MPoint ptResult;

    ptResult.x = (ptIn.x + 1000.0f) / 2000.0f * DMBG_SIZE.w;
    ptResult.y = (ptIn.y + 1000.0f) / 2000.0f * DMBG_SIZE.h;

    return ptResult;
}

bool
GF_HAL_IMP::_validateAFPoint(MPoint &ptIn)
{
    bool ret = true;
    if(ptIn.x < 0) {
        MY_LOGW("Invalid AF point: (%d, %d)", ptIn.x, ptIn.y);
        ptIn.x = 0;
        ret = false;
    } else if(ptIn.x >= DMBG_SIZE.w) {
        MY_LOGW("Invalid AF point: (%d, %d)", ptIn.x, ptIn.y);
        ptIn.x = DMBG_SIZE.w - 1;
        ret = false;
    }

    if(ptIn.y < 0) {
        MY_LOGW("Invalid AF point: (%d, %d)", ptIn.x, ptIn.y);
        ptIn.y = 0;
        ret = false;
    } else if(ptIn.y >= DMBG_SIZE.h) {
        MY_LOGW("Invalid AF point: (%d, %d)", ptIn.x, ptIn.y);
        ptIn.y = DMBG_SIZE.h - 1;
        ret = false;
    }

    return ret;
}

MPoint &
GF_HAL_IMP::_rotateAFPoint(MPoint &ptIn)
{
    MPoint newPt = ptIn;
    switch(StereoSettingProvider::getModuleRotation()) {
        case 90:
            newPt.x = DMBG_SIZE.h - ptIn.y;
            newPt.y = ptIn.x;
            break;
        case 180:
            newPt.x = DMBG_SIZE.w - ptIn.x;
            newPt.y = DMBG_SIZE.h - ptIn.y;
            break;
        case 270:
            newPt.x = ptIn.y;
            newPt.y = DMBG_SIZE.w - ptIn.x;
            break;
        default:
            break;
    }

    ptIn = newPt;
    return ptIn;
}

void
GF_HAL_IMP::_updateDACInfo(const size_t AF_INDEX, GFDacInfo &dacInfo)
{
    if(__pAFTable) {
        dacInfo.min = __pAFTable->af_dac_min;
        dacInfo.max = __pAFTable->af_dac_max;
        dacInfo.cur = __pAFTable->daf_vec[AF_INDEX].posture_dac;
        if(0 == dacInfo.cur) {
            dacInfo.cur = __pAFTable->daf_vec[AF_INDEX].af_dac_pos;
        }
    } else {
        MY_LOGE("AF Table not ready");
    }
}

void
GF_HAL_IMP::_setGFParams(GF_HAL_IN_DATA &gfHalParam)
{
    AutoProfileUtil profile(LOG_TAG, "Set Proc");

    // AF trigger table
    // +----------------------------------------------------------------------------+
    // | Case | Scenario   | AF Done                  |   ROI Change   | Trigger GF |
    // +------+------------+--------------------------+----------------+------------|
    // | 1    | FF Lens    | N/A                      | Don't care     | Per frame  |
    // +------+------------+--------------------------+----------------+------------|
    // | 2    | Capture    | Don't care               | Don't care     |     Y      |
    // | 3    | Preview/VR | By AF state              | Don't care     |     Y      |
    // | 4    | VR         | By AF state              | Y or 1st frame |     Y      |
    // | 5    | Preview/VR | Has FD                   | Don't care     |     Y      |
    // +----------------------------------------------------------------------------+
    if(gfHalParam.focalLensFactor > 0) {
        __procInfo.fb           = gfHalParam.focalLensFactor;
    } else {
        MY_LOGD("Invalid FB: %f, ignored", gfHalParam.focalLensFactor);
    }

    __updateZoomParam(gfHalParam.previewCropRatio, __procInfo);

    __procInfo.frame.sn = gfHalParam.ndd_timestamp;
    __procInfo.frame.id = __requestNumber;
    __procInfo.cOffset  = gfHalParam.convOffset;
    __procInfo.afInfo.touchTrigger = false;
    MPoint focusPoint;
    MRect focusRect;

    if(NULL == gfHalParam.fdData)
    {
        __procInfo.faceNum = 0;
    }
    else
    {
        __procInfo.faceNum = gfHalParam.fdData->number_of_faces;
        int MAX_FD_INDEX = std::min(__GF_MAX_FD_COUNT, __procInfo.faceNum);
        for(size_t f = 0; f < MAX_FD_INDEX; ++f)
        {
            __facesToFDinfo(gfHalParam.fdData, __procInfo.face[f], f);
        }
        FAST_LOG_PRINT;
    }

    const size_t AF_INDEX = __magicNumber % DAF_TBL_QLEN;
    if(__isAFSupported && __pAFTable != nullptr) {
        if(__magicNumber == __pAFTable->daf_vec[AF_INDEX].frm_mun) {
            const bool IS_AF_STABLE = __pAFTable->daf_vec[AF_INDEX].is_af_stable;
            FAST_LOGD("AF_INDEX %d, frame %d, AF stable %d, scenario %d, capture %d",
                      AF_INDEX, gfHalParam.magicNumber, IS_AF_STABLE,
                      gfHalParam.scenario, gfHalParam.isCapture);

            __procInfo.afInfo.touchTrigger = IS_AF_STABLE;
            _updateDACInfo(AF_INDEX, __procInfo.dacInfo);
            focusRect = _getAFRect(AF_INDEX);
            focusPoint.x = focusRect.p.x + focusRect.s.w/2;
            focusPoint.y = focusRect.p.y + focusRect.s.h/2;
        } else {
            MY_LOGW("AF frame number (%d) != GF(%d)",
                    __pAFTable->daf_vec[AF_INDEX].frm_mun, __magicNumber);
        }
    } else {    //FF
        focusPoint = __lastTouchPoint;
        MPoint touchPoint = _getTouchPoint(MPoint(gfHalParam.touchPosX, gfHalParam.touchPosY));
        _validateAFPoint(touchPoint);
        _rotateAFPoint(touchPoint);

        auto updateTouchPos = [&]() {
            focusPoint = touchPoint;
            __lastTouchPoint = touchPoint;
        };

        //Case 1: Trigger when ROI change
        if(__lastTouchPoint.x != touchPoint.x ||
           __lastTouchPoint.y != touchPoint.y)
        {
            FAST_LOGD("Update Touch Point (%d, %d) -> (%d, %d)",
                      __lastTouchPoint.x, __lastTouchPoint.y, touchPoint.x, touchPoint.y);
            updateTouchPos();
        } else {
            __hasFace = __procInfo.faceNum > 0;
            if(__hasFace) {
                focusPoint.x = focusRect.p.x + focusRect.s.w/2;
                focusPoint.y = focusRect.p.y + focusRect.s.h/2;
            }
        }

        //Always trigger for FF
        __procInfo.afInfo.touchTrigger = true;

        FAST_LOG_PRINT;
    }

    // if(__procInfo.afInfo.touchTrigger)
    {
        __procInfo.afInfo.touch.x = focusPoint.x;
        __procInfo.afInfo.touch.y = focusPoint.y;

        if(__isAFSupported && __pAFTable != nullptr) {
            __procInfo.afInfo.afType = static_cast<GF_AF_TYPE_ENUM>(__pAFTable->daf_vec[AF_INDEX].af_roi_sel);
            __procInfo.afInfo.box0.x = focusRect.p.x;
            __procInfo.afInfo.box0.y = focusRect.p.y;
            __procInfo.afInfo.box1.x = focusRect.p.x + focusRect.s.w - 1;
            __procInfo.afInfo.box1.y = focusRect.p.y + focusRect.s.h - 1;
        } else {
            if(__hasFace) {
                __procInfo.afInfo.afType = GF_AF_FD;
            } else {
                __procInfo.afInfo.afType = GF_AF_AP;
            }

            __procInfo.afInfo.box0.x = focusPoint.x;
            __procInfo.afInfo.box0.y = focusPoint.y;
            __procInfo.afInfo.box1.x = focusPoint.x;
            __procInfo.afInfo.box1.y = focusPoint.y;
        }
    }

    __procInfo.dof = StereoTuningProvider::getGFDoFValue(gfHalParam.dofLevel, __eScenario);
    __procInfo.numOfBuffer = 1; //depthMap

    MSize size = gfHalParam.depthMap->getImgSize();
    MY_LOGE_IF((size.w != __depthInputArea.size.w || size.h != __depthInputArea.size.h),
               "Size mismatch: buffer size %dx%d, depth size %dx%d",
               size.w, size.h, __depthInputArea.size.w, __depthInputArea.size.h);

    __procInfo.bufferInfo[0].type       = GF_BUFFER_TYPE_DEPTH;
    __procInfo.bufferInfo[0].format     = GF_IMAGE_YONLY;
    __procInfo.bufferInfo[0].width      = __initInfo.inputWidth;
    __procInfo.bufferInfo[0].height     = __initInfo.inputHeight;
    __procInfo.bufferInfo[0].stride     = size.w;
    __procInfo.bufferInfo[0].planeAddr0 = (PEL*)gfHalParam.depthMap->getBufVA(0);
    __procInfo.bufferInfo[0].planeAddr1 = NULL;
    __procInfo.bufferInfo[0].planeAddr2 = NULL;
    __procInfo.bufferInfo[0].planeAddr3 = NULL;

    if(gfHalParam.confidenceMap) {
        __procInfo.numOfBuffer = 2; //confidence map

#if (HW_CONF_FORMAT==1)
        __procInfo.bufferInfo[1].type       = GF_BUFFER_TYPE_CONF_IN;
#elif (HW_CONF_FORMAT==2)
        __procInfo.bufferInfo[1].type       = GF_BUFFER_TYPE_CONF_REPT;
#elif (HW_CONF_FORMAT==3)
        __procInfo.bufferInfo[1].type       = GF_BUFFER_TYPE_CONF_REPT_FLAT;
#else
        __procInfo.bufferInfo[1].type       = GF_BUFFER_TYPE_CONF_IN;
#endif

        __procInfo.bufferInfo[1].format     = GF_IMAGE_YONLY;
#if (HAS_HW_DPE < 2)
        _removePadding(gfHalParam.confidenceMap, __confidenceMap.get(), __confidenceArea);
        __procInfo.bufferInfo[1].width      = __confidenceMap->getImgSize().w;
        __procInfo.bufferInfo[1].height     = __confidenceMap->getImgSize().h;
        __procInfo.bufferInfo[1].stride     = __confidenceMap->getImgSize().w;
        __procInfo.bufferInfo[1].planeAddr0 = (PEL*)__confidenceMap.get()->getBufVA(0);
#else
        __procInfo.bufferInfo[1].width      = __initInfo.inputWidth;
        __procInfo.bufferInfo[1].height     = __initInfo.inputHeight;
        __procInfo.bufferInfo[1].stride     = gfHalParam.confidenceMap->getImgSize().w;
        __procInfo.bufferInfo[1].planeAddr0 = (PEL*)gfHalParam.confidenceMap->getBufVA(0);
#endif
        __procInfo.bufferInfo[1].planeAddr1 = NULL;
        __procInfo.bufferInfo[1].planeAddr2 = NULL;
        __procInfo.bufferInfo[1].planeAddr3 = NULL;
    }

    vector<IImageBuffer *>::iterator it = gfHalParam.images.begin();
    IImageBuffer *image = NULL;
    StereoArea areaMYS = StereoSizeProvider::getInstance()->getBufferSize(E_MY_S, __eScenario);
    MSize szMYSContent = areaMYS.contentSize();
    for(;it != gfHalParam.images.end(); ++it, ++__procInfo.numOfBuffer) {
        if( (*it)->getImgFormat() == eImgFmt_YV12 )
        {
            image = *it;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].format = GF_IMAGE_YV12;
        }
        else if( (*it)->getImgFormat() == eImgFmt_YUY2 )
        {
            image = *it;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].format = GF_IMAGE_YUY2;
        }
        else if( (*it)->getImgFormat() == eImgFmt_NV12 )
        {
            image = *it;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].format = GF_IMAGE_NV12;
        }
        else
        {
            sp<IImageBuffer> transformedImage;
            StereoDpUtil::allocImageBuffer(LOG_TAG, eImgFmt_YUY2, (*it)->getImgSize(), !IS_ALLOC_GB, transformedImage);
            StereoDpUtil::transformImage(*it, transformedImage.get());
            __transformedImages.push_back(transformedImage);
            image = transformedImage.get();
            __procInfo.bufferInfo[__procInfo.numOfBuffer].format = GF_IMAGE_YUY2;
        }

        __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr0 = (PEL*)image->getBufVA(0);
        if(image->getPlaneCount() == 2) {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr1 = (PEL*)image->getBufVA(1);
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr2 = NULL;
        } else if(image->getPlaneCount() >= 3) {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr1 = (PEL*)image->getBufVA(1);
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr2 = (PEL*)image->getBufVA(2);
        } else {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr1 = NULL;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr2 = NULL;
        }
        __procInfo.bufferInfo[__procInfo.numOfBuffer].planeAddr3 = NULL;

        if(size.w * 2 == image->getImgSize().w) {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].type   = GF_BUFFER_TYPE_DS_2X;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].width  = szMYSContent.w * 2;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].height = szMYSContent.h * 2;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].stride = areaMYS.size.w * 2;
        } else if(size.w * 4 == image->getImgSize().w) {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].type   = GF_BUFFER_TYPE_DS_4X;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].width  = szMYSContent.w * 4;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].height = szMYSContent.h * 4;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].stride = areaMYS.size.w * 4;
        } else {
            __procInfo.bufferInfo[__procInfo.numOfBuffer].type   = GF_BUFFER_TYPE_DS;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].width  = szMYSContent.w;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].height = szMYSContent.h;
            __procInfo.bufferInfo[__procInfo.numOfBuffer].stride = areaMYS.size.w;
        }
    }

    __logSetProcData();
    __dumpProcInfo();

    __pGfDrv->FeatureCtrl(GF_FEATURE_SET_PROC_INFO, &__procInfo, NULL);
}

void
GF_HAL_IMP::_runGF(GF_HAL_OUT_DATA &gfHalOutput)
{
    AutoProfileUtil profile(LOG_TAG, "Run GF");
    //================================
    //  Run GF
    //================================
    __pGfDrv->Main();

    //================================
    //  Get result
    //================================
    ::memset(&__resultInfo, 0, sizeof(GFResultInfo));
    {
        AutoProfileUtil profile(LOG_TAG, "Get Result");
        __pGfDrv->FeatureCtrl(GF_FEATURE_GET_RESULT, NULL, &__resultInfo);
    }
    __logGFResult();
    __dumpResultInfo();
    __dumpWorkingBuffer();

    //================================
    //  Copy result to GF_HAL_OUT_DATA
    //================================
    for(MUINT32 i = 0; i < __resultInfo.numOfBuffer; i++) {
        if(GF_BUFFER_TYPE_BMAP == __resultInfo.bufferInfo[i].type) {
            if(gfHalOutput.dmbg) {
                AutoProfileUtil profile(LOG_TAG, "Copy BMAP");
                _rotateResult(__resultInfo.bufferInfo[i], gfHalOutput.dmbg);
            }
        } else if(GF_BUFFER_TYPE_DMAP == __resultInfo.bufferInfo[i].type) {
            if(gfHalOutput.depthMap) {
                AutoProfileUtil profile(LOG_TAG, "Copy DepthMap");
                _rotateResult(__resultInfo.bufferInfo[i], gfHalOutput.depthMap);
            }
        }
    }

    // N3D Hal instance will be created by N3DNode (construct and init)
    // GF_HAL should get N3D Hal instance here to prevent timeing issue
    if (__n3d == nullptr) {
      __n3d = N3D_HAL::createInstance(__eScenario);
    }
    __n3d->setConvergence(__resultInfo.nvram, sizeof(__resultInfo.nvram));

    _clearTransformedImages();
}

void
GF_HAL_IMP::_clearTransformedImages()
{
    vector<sp<IImageBuffer>>::iterator it = __transformedImages.begin();
    for(; it != __transformedImages.end(); ++it) {
        if(it->get()) {
            StereoDpUtil::freeImageBuffer(LOG_TAG, *it);
        }
        *it = NULL;
    }

    __transformedImages.clear();
}

bool
GF_HAL_IMP::_rotateResult(GFBufferInfo &gfResult, IImageBuffer *targetBuffer)
{
    if(NULL == targetBuffer ||
       NULL == gfResult.planeAddr0)
    {
        return false;
    }

    MSize targetSize = targetBuffer->getImgSize();
    if((int)(gfResult.width*gfResult.height) != targetSize.w * targetSize.h) {
        MY_LOGW("Size mismatch, ignore rotation");
        return false;
    }

    int targetRotation = 0;
    switch(StereoSettingProvider::getModuleRotation())
    {
    case 90:
        targetRotation = 270;
        break;
    case  270:
        targetRotation = 90;
        break;
    case 180:
        targetRotation = 180;
        break;
    case 0:
    default:
        break;
    }

    if(__outputDepthmap &&
       !StereoSettingProvider::is3rdParty() &&
       !StereoSettingProvider::isMTKDepthmapMode())
    {
        //When output depthmap in tk flow, the depthmap is for packing,
        //so we don't rotate result
        targetRotation = 0;
    }

    return StereoDpUtil::rotateBuffer(LOG_TAG,
                                      gfResult.planeAddr0,
                                      MSize(gfResult.width, gfResult.height),
                                      (MUINT8*)targetBuffer->getBufVA(0),
                                      targetRotation);
}

MPoint
GF_HAL_IMP::__faceRectToTriggerPoint(MRect &rect)
{
    MPoint p;

    if(StereoSettingProvider::stereoProfile() == STEREO_SENSOR_PROFILE_FRONT_FRONT)
    {
        //NOTICE: front ducal cam not verified yet
        p.y = ((rect.p.x + rect.s.w/2 + 1000.0f)/2000.0f) * DMBG_SIZE.w;
        p.x = (1.0f - (rect.p.y + rect.s.h/2 + 1000.0f)/2000.0f) * DMBG_SIZE.h;
    }
    else
    {
        p.x = (rect.p.x + rect.s.w/2+1000.0f)/2000.0f * DMBG_SIZE.w;
        p.y = (rect.p.y + rect.s.h/2+1000.0f)/2000.0f * DMBG_SIZE.h;
        p = _rotateAFPoint(p);
    }

    return p;
}

void
GF_HAL_IMP::__facePointToGFPoint(int inX, int inY, int &outX, int &outY)
{
    auto &param = __getFDTransformParam(__procInfo.zoomRatioP1);
    MPoint pt(inX, inY);
    pt.x = (pt.x - param.offsetX) * param.scaleW;
    pt.y = (pt.y - param.offsetY) * param.scaleH;
    pt = StereoHAL::rotatePoint(pt, DMBG_SIZE, StereoSettingProvider::getModuleRotation());
    outX = pt.x;
    outY = pt.y;
}

void
GF_HAL_IMP::__facesToFDinfo(MtkCameraFaceMetadata *fd, GFFdInfo &fdInfo, size_t fdIndex)
{
    if(NULL == fd || fdIndex >= __GF_MAX_FD_COUNT)
    {
        return;
    }

    fdInfo.conf = fd->faces[fdIndex].score;
    fdInfo.rip  = fd->fld_rip[fdIndex];
    fdInfo.rop  = fd->fld_rop[fdIndex];
    fdInfo.visiblePercent = fd->visible_percent[fdIndex];

    __facePointToGFPoint(fd->faces[fdIndex].rect[0], fd->faces[fdIndex].rect[1],
                         fdInfo.box0.x, fdInfo.box0.y);

    __facePointToGFPoint(fd->faces[fdIndex].rect[2], fd->faces[fdIndex].rect[3],
                         fdInfo.box1.x, fdInfo.box1.y);

    __facePointToGFPoint(fd->leyex0[fdIndex], fd->leyey0[fdIndex],
                         fdInfo.eyeL0.x, fdInfo.eyeL0.y);
    __facePointToGFPoint(fd->leyex1[fdIndex], fd->leyey1[fdIndex],
                         fdInfo.eyeL1.x, fdInfo.eyeL1.y);
    __facePointToGFPoint(fd->reyex0[fdIndex], fd->reyey0[fdIndex],
                         fdInfo.eyeR0.x, fdInfo.eyeR0.y);
    __facePointToGFPoint(fd->reyex1[fdIndex], fd->reyey1[fdIndex],
                         fdInfo.eyeR1.x, fdInfo.eyeR1.y);

    __facePointToGFPoint(fd->nosex[fdIndex], fd->nosex[fdIndex],
                         fdInfo.nose.x, fdInfo.nose.y);

    __facePointToGFPoint(fd->mouthx0[fdIndex], fd->mouthy0[fdIndex],
                         fdInfo.mouth0.x, fdInfo.mouth0.y);
    __facePointToGFPoint(fd->mouthx1[fdIndex], fd->mouthy1[fdIndex],
                         fdInfo.mouth1.x, fdInfo.mouth1.y);
}

MRect
GF_HAL_IMP::__faceToMRect(MtkCameraFace &face)
{
    MRect result;

    //MtkCameraFace::rect is left, top, right, bottom, range is -1000~1000
    result.p.x = face.rect[0];
    result.p.y = face.rect[1];
    result.s.w = abs(face.rect[2]-face.rect[0]);
    result.s.h = abs(face.rect[3]-face.rect[1]);

    return result;
}

MRect
GF_HAL_IMP::__faceRectToFocusRect(MRect &rect)
{
    auto &param = __getFDTransformParam(__procInfo.zoomRatioP1);
    MRect focusRect;

    focusRect.p.x = (rect.p.x - param.offsetX) * param.scaleW;
    focusRect.p.y = (rect.p.y - param.offsetY) * param.scaleH;
    focusRect.s.w = rect.s.w * param.scaleW;
    focusRect.s.h = rect.s.h * param.scaleH;
    focusRect = StereoHAL::rotateRect(focusRect, DMBG_SIZE, StereoSettingProvider::getModuleRotation());

    return focusRect;
}

GF_HAL_IMP::CoordTransformParam &
GF_HAL_IMP::__getFDTransformParam(float main1RoomRatio)
{
    auto it = __fdTransformMap.find(main1RoomRatio);
    if(it != __fdTransformMap.end()) {
        return it->second;
    }

    MRect tgCrop;
    StereoSizeProvider::getInstance()->getPass1ActiveArrayCrop(eSTEREO_SENSOR_MAIN1, tgCrop, true, 1.0f/main1RoomRatio);
    __fdTransformMap.emplace(main1RoomRatio, CoordTransformParam(tgCrop, DMBG_SIZE));
    return __fdTransformMap[main1RoomRatio];
}

GF_HAL_IMP::CoordTransformParam &
GF_HAL_IMP::__getAFTransformParam(float main1RoomRatio)
{
    auto it = __afTransformMap.find(main1RoomRatio);
    if(it != __afTransformMap.end()) {
        return it->second;
    }

    MUINT32 junkStride;
    MSize   szMain1RRZO;
    MRect   tgCrop1x;
    MRect   tgCropZoomed;
    StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN1,
                                                       eImgFmt_FG_BAYER10,
                                                       EPortIndex_RRZO,
                                                       __eScenario,
                                                       1.0f/main1RoomRatio
                                                     },
                                                     //below are outputs
                                                     tgCropZoomed,
                                                     szMain1RRZO,
                                                     junkStride);

    StereoSizeProvider::getInstance()->getPass1Size( { eSTEREO_SENSOR_MAIN1,
                                                       eImgFmt_FG_BAYER10,
                                                       EPortIndex_RRZO,
                                                       __eScenario,
                                                       1.0f
                                                     },
                                                     //below are outputs
                                                     tgCrop1x,
                                                     szMain1RRZO,
                                                     junkStride);

    CoordTransformParam param(tgCrop1x.s, tgCropZoomed.p, DMBG_SIZE);
    __afTransformMap.emplace(main1RoomRatio, param);
    return __afTransformMap[main1RoomRatio];
}

void
GF_HAL_IMP::__updateZoomParam(float zoomRatio, GFProcInfo &procInfo)
{
    procInfo.zoomRatioP1 = 1.0f;
    procInfo.zoomRatioP2 = 1.0f;

    if(zoomRatio < 0.0f ||
       zoomRatio > 1.0f)
    {
        MY_LOGW("Invalid preview crop ratio %f", zoomRatio);
        return;
    }

    if(zoomRatio >= __MIN_P1_CROP_RATIO) {
        procInfo.zoomRatioP1 = 1.0f/zoomRatio;
    } else {
        procInfo.zoomRatioP1 = 1.0f/__MIN_P1_CROP_RATIO;
        procInfo.zoomRatioP2 = __MIN_P1_CROP_RATIO/zoomRatio;
    }
}

void
GF_HAL_IMP::_removePadding(IImageBuffer *src, IImageBuffer *dst, StereoArea &srcArea)
{
    MSize dstSize = dst->getImgSize();
    if(dstSize.w > srcArea.size.w ||
       dstSize.h > srcArea.size.h)
    {
        return;
    }

    MUINT8 *pImgIn  = (MUINT8*)src->getBufVA(0);
    MUINT8 *pImgOut = (MUINT8*)dst->getBufVA(0);
    ::memset(pImgOut, 0, dstSize.w * dstSize.h);
    //Copy Y
    pImgIn = pImgIn + srcArea.size.w * srcArea.startPt.y + srcArea.startPt.x;
    for(int i = dstSize.h-1; i >= 0; --i, pImgOut += dstSize.w, pImgIn += srcArea.size.w)
    {
        ::memcpy(pImgOut, pImgIn, dstSize.w);
    }

    //Copy UV
    if(src->getPlaneCount() == 3 &&
       dst->getPlaneCount() == 3)
    {
        dstSize.w /= 2;
        dstSize.h /= 2;
        srcArea /= 2;
        for(int p = 1; p < 3; ++p) {
            pImgIn = (MUINT8*)src->getBufVA(p);
            pImgOut = (MUINT8*)dst->getBufVA(p);
            ::memset(pImgOut, 0, dstSize.w * dstSize.h);

            pImgIn = pImgIn + srcArea.size.w * srcArea.startPt.y + srcArea.startPt.x;
            for(int i = dstSize.h-1; i >= 0; --i, pImgOut += dstSize.w, pImgIn += srcArea.size.w)
            {
                ::memcpy(pImgOut, pImgIn, dstSize.w);
            }
        }
    }
}

void
GF_HAL_IMP::__runGFAFThread()
{
    do {
        std::unique_lock<std::mutex> lk(__GFAFMutex);
        //Note: wait will unlock lk and then wait
        __GFAFCondVar.wait(lk, [this]{return __wakeupGFAF;});
        __wakeupGFAF = false;

        if(__leaveGFAF) {
            MY_LOGD("Leave GFAF thread");
            break;
        }

        __pGfDrv->FeatureCtrl(GF_FEATURE_STATISTICS, NULL, NULL);
        __pGfDrv->FeatureCtrl(GF_FEATURE_GET_STATISTICS_RESULT, NULL, &__resultInfo.bokehAf);
        // TODO: pass result to AF

        lk.unlock();
    } while(1);
}

bool
GF_HAL_IMP::__loadConvergence()
{
    MINT32 err = 0;
    int32_t main1DevIdx = 0, main2DevIdx = 0;
    StereoSettingProvider::getStereoSensorDevIndex(main1DevIdx, main2DevIdx);
    auto pNvBufUtil = MAKE_NvBufUtil();
    static NVRAM_CAMERA_GEOMETRY_STRUCT* pNvramData;
    err = (!pNvBufUtil) ? -1 : pNvBufUtil->getBufAndRead(CAMERA_NVRAM_DATA_GEOMETRY, main1DevIdx, (void*&)pNvramData);
    if(err ||
       NULL == pNvramData)
    {
        MY_LOGE("Read NVRAM fail, data: %p pNvBufUtil:%p", pNvramData, pNvBufUtil);
        return false;
    }

    MY_LOGD_IF(LOG_ENABLED, "NVRAM %p", pNvramData);

    //Reset NVRAM if needed
    if(1 == checkStereoProperty("vendor.STEREO.reset_nvram")) {
        MY_LOGD("Reset NVRAM");
        ::memset(pNvramData, 0, sizeof(NVRAM_CAMERA_GEOMETRY_STRUCT));
    }

    //Load convergence
    const size_t CONV_SIZE = sizeof(__initInfo.nvram);
    MUINT8 *convAddr = reinterpret_cast<MUINT8 *>(&(pNvramData->StereoNvramData.StereoData[0]))
                       + sizeof(pNvramData->StereoNvramData.StereoData) - CONV_SIZE;
    ::memcpy(__initInfo.nvram, convAddr, CONV_SIZE);
    return true;
}