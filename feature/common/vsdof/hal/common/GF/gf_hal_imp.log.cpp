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

void
GF_HAL_IMP::__logInitData()
{
    if(!LOG_ENABLED) {
        return;
    }

    FAST_LOGD("========= GF Init Info =========");
    //__initInfo
    FAST_LOGD("[GF Mode]      %d", __initInfo.gfMode);
    FAST_LOGD("[InputWidth]   %d", __initInfo.inputWidth);
    FAST_LOGD("[InputHeight]  %d", __initInfo.inputHeight);
    //
    FAST_LOGD("[OutputWidth]  %d", __initInfo.outputWidth);
    FAST_LOGD("[OutputHeight] %d", __initInfo.outputHeight);
    //
    FAST_LOGD("[baseline]     %f", __initInfo.cam.baseline);
    FAST_LOGD("[focalLength]  %f", __initInfo.cam.focalLength);
    FAST_LOGD("[fNumber]      %f", __initInfo.cam.fNumber);
    FAST_LOGD("[sensorWidth]  %f", __initInfo.cam.sensorWidth);
    FAST_LOGD("[pixelArrayW]  %d", __initInfo.cam.pixelArrayW);
    FAST_LOGD("[sensorSizeW]  %d", __initInfo.cam.sensorSizeW);
    FAST_LOGD("[sensorScaleW] %d", __initInfo.cam.sensorScaleW);
    FAST_LOGD("[rrzUsageW]    %d", __initInfo.cam.rrzUsageW);
    FAST_LOGD("[renderWidth]  %d", __initInfo.cam.renderWidth);
    //=== Init tuning info ===
    FAST_LOGD("[CoreNumber]   %d", __initInfo.pTuningInfo->coreNumber);
    std::ostringstream oss;
    for(int i = 0; i < __initInfo.pTuningInfo->clrTblSize; ++i) {
        oss << __initInfo.pTuningInfo->clrTbl[i] << " ";
    }
    FAST_LOGD("[Tuning Table] %s", oss.str().c_str());

    oss.clear();
    oss.str("");
    for(int i = 0; i < __initInfo.pTuningInfo->ctrlPointNum; ++i) {
        oss << __initInfo.pTuningInfo->dispCtrlPoints[i] << " ";
    }
    FAST_LOGD("[dispCtrlPoints] %s", oss.str().c_str());

    oss.clear();
    oss.str("");
    for(int i = 0; i < __initInfo.pTuningInfo->ctrlPointNum; ++i) {
        oss << __initInfo.pTuningInfo->blurGainTable[i] << " ";
    }
    FAST_LOGD("[blurGainTable] %s", oss.str().c_str());

#ifdef GF_CUSTOM_PARAM
    FAST_LOGD("[TuningInfo.NumOfParam]    %d", __initInfo.pTuningInfo->NumOfParam);
    for(MUINT32 j = 0; j < __initInfo.pTuningInfo->NumOfParam; ++j) {
        FAST_LOGD("[TuningInfo.params][%d]     %s: %d", j,
                  __initInfo.pTuningInfo->params[j].key,
                  __initInfo.pTuningInfo->params[j].value);
    }
#endif

    FAST_LOG_PRINT;
}

void
GF_HAL_IMP::__logSetProcData()
{
    if(!LOG_ENABLED) {
        return;
    }

    FAST_LOGD("========= GF Proc Data =========");
    //__procInfo
    FAST_LOGD("[Magic]        %d", __magicNumber);
    FAST_LOGD("[Request]      %d", __requestNumber);
    FAST_LOGD("[TouchTrigger] %d", __procInfo.afInfo.touchTrigger);
    FAST_LOGD("[TouchPos]     (%d, %d)", __procInfo.afInfo.touch.x, __procInfo.afInfo.touch.y);
    FAST_LOGD("[DoF Level]    %d", __procInfo.dof);
    FAST_LOGD("[DAC]          %d (%d-%d)", __procInfo.dacInfo.cur, __procInfo.dacInfo.min, __procInfo.dacInfo.max);
    FAST_LOGD("[ConvOffset]   %f", __procInfo.cOffset);
    FAST_LOGD("[zoomRatioP1]  %f", __procInfo.zoomRatioP1);
    FAST_LOGD("[zoomRatioP2]  %f", __procInfo.zoomRatioP2);
    FAST_LOGD("[FB]           %f", __procInfo.fb);
    string focusType;
    switch(__procInfo.afInfo.afType) {
    case GF_AF_NONE:
        focusType = "None";
        break;
    case GF_AF_AP:
        focusType = "AP";
        break;
    case GF_AF_OT:
        focusType = "OT";
        break;
    case GF_AF_FD:
        focusType = "FD";
        break;
    case GF_AF_CENTER:
        focusType = "Center";
        break;
    case GF_AF_DEFAULT:
    default:
        focusType = "Default";
        break;
    }
    FAST_LOGD("[Focus Type]   %d(%s)", __procInfo.afInfo.afType, focusType.c_str());
    FAST_LOGD("[Focus ROI]    (%d, %d) (%d, %d)", __procInfo.afInfo.box0.x,
                                                  __procInfo.afInfo.box0.y,
                                                  __procInfo.afInfo.box1.x,
                                                  __procInfo.afInfo.box1.y);

    // Face
    FAST_LOGD("[faceNum]      %d", __procInfo.faceNum);
    if(__procInfo.faceNum > 0)
    {
        for(size_t i = 0; i < __procInfo.faceNum; ++i)
        {
            FAST_LOGD("   [Face][%d] conf %d, rip %d, rop %d, visiblePercent %d\n"
                      "              box0   (% 4d, % 4d)\n"
                      "              box1   (% 4d, % 4d)\n"
                      "              eyeL0  (% 4d, % 4d)\n"
                      "              eyeL1  (% 4d, % 4d)\n"
                      "              eyeR0  (% 4d, % 4d)\n"
                      "              eyeR1  (% 4d, % 4d)\n"
                      "              nose   (% 4d, % 4d)\n"
                      "              mouth0 (% 4d, % 4d)\n"
                      "              mouth1 (% 4d, % 4d)"
                      , i
                      , __procInfo.face[i].conf
                      , __procInfo.face[i].rip
                      , __procInfo.face[i].rop
                      , __procInfo.face[i].visiblePercent
                      , __procInfo.face[i].box0.x
                      , __procInfo.face[i].box0.y
                      , __procInfo.face[i].box1.x
                      , __procInfo.face[i].box1.y
                      , __procInfo.face[i].eyeL0.x
                      , __procInfo.face[i].eyeL0.y
                      , __procInfo.face[i].eyeL1.x
                      , __procInfo.face[i].eyeL1.y
                      , __procInfo.face[i].eyeR0.x
                      , __procInfo.face[i].eyeR0.y
                      , __procInfo.face[i].eyeR1.x
                      , __procInfo.face[i].eyeR1.y
                      , __procInfo.face[i].nose.x
                      , __procInfo.face[i].nose.y
                      , __procInfo.face[i].mouth0.x
                      , __procInfo.face[i].mouth0.y
                      , __procInfo.face[i].mouth1.x
                      , __procInfo.face[i].mouth1.y);
        }
    }

    for(unsigned int i = 0; i < __procInfo.numOfBuffer; i++) {
        __logGFBufferInfo(__procInfo.bufferInfo[i], (int)i);
    }

    FAST_LOG_PRINT;
}

void
GF_HAL_IMP::__logGFResult()
{
    if(!LOG_ENABLED) {
        return;
    }

    FAST_LOGD("========= GF Result =========");
    //__resultInfo
    FAST_LOGD("[Return code] %d", __resultInfo.RetCode);
    FAST_LOGD("[focusStatus] %d", __resultInfo.focusStatus);
    FAST_LOGD("[bgStatus]    %u", __resultInfo.bgStatus);
    // FAST_LOGD("[lowConf]     %u", __resultInfo.lowConf);
    // FAST_LOGD("[tracking]    %u", __resultInfo.tracking);
    FAST_LOGD("[transition]  %f", __resultInfo.transition);
    // FAST_LOGD("[convergence] %d", __resultInfo.convergence);
    FAST_LOGD("[dof]         %u", __resultInfo.dof);
    FAST_LOGD("[focus]       %u", __resultInfo.focus);
    FAST_LOGD("[clr]         %u", __resultInfo.clr);
    FAST_LOGD("[gain]        %u", __resultInfo.gain);

    for(unsigned int i = 0; i < __resultInfo.numOfBuffer; i++) {
        __logGFBufferInfo(__resultInfo.bufferInfo[i], (int)i);
    }

    FAST_LOG_PRINT;
}

void
GF_HAL_IMP::__logGFBufferInfo(const GFBufferInfo &buf, int index)
{
    if(!LOG_ENABLED) {
        return;
    }

    if(index >= 0) {
        FAST_LOGD("[Buffer %d][Type]          %d", index, buf.type);
        FAST_LOGD("[Buffer %d][Format]        %d", index, buf.format);
        FAST_LOGD("[Buffer %d][Width]         %d", index, buf.width);
        FAST_LOGD("[Buffer %d][Height]        %d", index, buf.height);
        FAST_LOGD("[Buffer %d][Stride]        %d", index, buf.stride);
        FAST_LOGD("[Buffer %d][PlaneAddr0]    %p", index, buf.planeAddr0);
        FAST_LOGD("[Buffer %d][PlaneAddr1]    %p", index, buf.planeAddr1);
        FAST_LOGD("[Buffer %d][PlaneAddr2]    %p", index, buf.planeAddr2);
        FAST_LOGD("[Buffer %d][PlaneAddr3]    %p", index, buf.planeAddr3);
    } else {
        FAST_LOGD("[Type]          %d", buf.type);
        FAST_LOGD("[Format]        %d", buf.format);
        FAST_LOGD("[Width]         %d", buf.width);
        FAST_LOGD("[Height]        %d", buf.height);
        FAST_LOGD("[Stride]        %d", buf.stride);
        FAST_LOGD("[PlaneAddr0]    %p", buf.planeAddr0);
        FAST_LOGD("[PlaneAddr1]    %p", buf.planeAddr1);
        FAST_LOGD("[PlaneAddr2]    %p", buf.planeAddr2);
        FAST_LOGD("[PlaneAddr3]    %p", buf.planeAddr3);
    }
}
