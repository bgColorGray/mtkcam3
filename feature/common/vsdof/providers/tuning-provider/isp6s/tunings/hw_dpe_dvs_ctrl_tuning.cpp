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
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "StereoTuningProvider_DVS_CTRL"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "hw_dpe_dvs_ctrl_tuning.h"

#if defined(__func__)
#undef __func__
#endif
#define __func__ __FUNCTION__

#define MY_LOGD(fmt, arg...)    CAM_ULOGMD("[%s]" fmt, __func__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_ULOGMI("[%s]" fmt, __func__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_ULOGMW("[%s] WRN(%5d):" fmt, __func__, __LINE__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_ULOGME("[%s] %s ERROR(%5d):" fmt, __func__,__FILE__, __LINE__, ##arg)

#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }

HW_DPE_DVS_CTRL_Tuning::~HW_DPE_DVS_CTRL_Tuning()
{
    for(auto &t : __tuningMap) {
        if(t.second) {
            delete t.second;
        }
    }
}

bool
HW_DPE_DVS_CTRL_Tuning::retrieveTuningParams(TuningQuery_T &query)
{
    DPE_Config *dst = (DPE_Config *)query.results[STEREO_TUNING_NAME[E_TUNING_HW_DPE]];
    if(NULL == dst) {
        MY_LOGE("Cannot get target");
        return false;
    }

    DPE_Config *src = NULL;
    ENUM_STEREO_SCENARIO scenario = static_cast<ENUM_STEREO_SCENARIO>(query.intParams[QUERY_KEY_SCENARIO]);
    if(StereoSettingProvider::isActiveStereo() &&
       __tuningMap.find(QUERY_KEY_ACTIVE) != __tuningMap.end())
    {
        src = __tuningMap[QUERY_KEY_ACTIVE];
    }
    else if(eSTEREO_SCENARIO_PREVIEW == scenario &&
            __tuningMap.find(QUERY_KEY_PREVIEW) != __tuningMap.end())
    {
        src = __tuningMap[QUERY_KEY_PREVIEW];
    }
    else if(eSTEREO_SCENARIO_RECORD == scenario &&
            (__tuningMap.find(QUERY_KEY_RECORD) != __tuningMap.end() ||
             __tuningMap.find(QUERY_KEY_PREVIEW) != __tuningMap.end()))
    {
        if(__tuningMap.find(QUERY_KEY_RECORD) != __tuningMap.end())
        {
            src = __tuningMap[QUERY_KEY_RECORD];
        }
        else
        {
            src = __tuningMap[QUERY_KEY_PREVIEW];
        }
    }
    else
    {
        src = __tuningMap.begin()->second;
    }

    if(NULL == src) {
        MY_LOGE("Cannot get source");
        return false;
    }

    dst->Dpe_engineSelect                                 = src->Dpe_engineSelect;
    dst->Dpe_is16BitMode                                  = src->Dpe_is16BitMode;
    dst->Dpe_DVSSettings.mainEyeSel                       = src->Dpe_DVSSettings.mainEyeSel;
    dst->Dpe_DVSSettings.is_pd_mode                       = src->Dpe_DVSSettings.is_pd_mode;
    dst->Dpe_DVSSettings.pd_frame_num                     = src->Dpe_DVSSettings.pd_frame_num;
    dst->Dpe_DVSSettings.pd_st_x                          = src->Dpe_DVSSettings.pd_st_x;
    dst->Dpe_DVSSettings.frmWidth                         = src->Dpe_DVSSettings.frmWidth;
    dst->Dpe_DVSSettings.frmHeight                        = src->Dpe_DVSSettings.frmHeight;
    dst->Dpe_DVSSettings.L_engStart_x                     = src->Dpe_DVSSettings.L_engStart_x;
    dst->Dpe_DVSSettings.R_engStart_x                     = src->Dpe_DVSSettings.R_engStart_x;
    dst->Dpe_DVSSettings.engStart_y                       = src->Dpe_DVSSettings.engStart_y;
    dst->Dpe_DVSSettings.engWidth                         = src->Dpe_DVSSettings.engWidth;
    dst->Dpe_DVSSettings.engHeight                        = src->Dpe_DVSSettings.engHeight;
    dst->Dpe_DVSSettings.engStart_y                       = src->Dpe_DVSSettings.engStart_y;
    dst->Dpe_DVSSettings.occWidth                         = src->Dpe_DVSSettings.occWidth;
    dst->Dpe_DVSSettings.occStart_x                       = src->Dpe_DVSSettings.occStart_x;
    dst->Dpe_DVSSettings.pitch                            = src->Dpe_DVSSettings.pitch;
    dst->Dpe_DVSSettings.SubModule_EN.sbf_en              = src->Dpe_DVSSettings.SubModule_EN.sbf_en;
    dst->Dpe_DVSSettings.SubModule_EN.conf_en             = src->Dpe_DVSSettings.SubModule_EN.conf_en;
    dst->Dpe_DVSSettings.SubModule_EN.occ_en              = src->Dpe_DVSSettings.SubModule_EN.occ_en;
    dst->Dpe_DVSSettings.Iteration.y_IterTimes            = src->Dpe_DVSSettings.Iteration.y_IterTimes;
    dst->Dpe_DVSSettings.Iteration.y_IterStartDirect_0    = src->Dpe_DVSSettings.Iteration.y_IterStartDirect_0;
    dst->Dpe_DVSSettings.Iteration.y_IterStartDirect_1    = src->Dpe_DVSSettings.Iteration.y_IterStartDirect_1;
    dst->Dpe_DVSSettings.Iteration.x_IterStartDirect_0    = src->Dpe_DVSSettings.Iteration.x_IterStartDirect_0;
    dst->Dpe_DVSSettings.Iteration.x_IterStartDirect_1    = src->Dpe_DVSSettings.Iteration.x_IterStartDirect_1;

    return true;
}

void
HW_DPE_DVS_CTRL_Tuning::_initFrameSize(DVS_Settings *setting, ENUM_STEREO_SCENARIO scenario)
{
    StereoArea area = StereoSizeProvider::getInstance()->getBufferSize(E_MV_Y, scenario);
    MSize contentSize = area.contentSize();
    setting->frmWidth     = area.size.w;
    setting->frmHeight    = area.size.h;
    setting->engStart_y   = area.startPt.y;
    setting->occWidth     = contentSize.w;
    setting->occStart_x   = area.startPt.x;
    setting->L_engStart_x = area.startPt.x;
    setting->R_engStart_x = 0;
    setting->engWidth     = setting->frmWidth - setting->L_engStart_x - setting->R_engStart_x;
    setting->engHeight    = contentSize.h;
    setting->pitch        = setting->frmWidth;
}

void
HW_DPE_DVS_CTRL_Tuning::_initDefaultValues()
{
    DPE_Config *tuningBuffer = new DPE_Config();

    tuningBuffer->Dpe_engineSelect            = MODE_DVS_DVP_BOTH;
    tuningBuffer->Dpe_is16BitMode             = 0;
    if(0 == StereoSettingProvider::getSensorRelativePosition()) {
        tuningBuffer->Dpe_DVSSettings.mainEyeSel = LEFT;
    } else {
        tuningBuffer->Dpe_DVSSettings.mainEyeSel = RIGHT;
    }

    tuningBuffer->Dpe_DVSSettings.is_pd_mode   = 0;
    tuningBuffer->Dpe_DVSSettings.pd_frame_num = 0;
    tuningBuffer->Dpe_DVSSettings.pd_st_x      = 0;
    _initFrameSize(&(tuningBuffer->Dpe_DVSSettings), eSTEREO_SCENARIO_PREVIEW);

    tuningBuffer->Dpe_DVSSettings.SubModule_EN.sbf_en              = 0;
    tuningBuffer->Dpe_DVSSettings.SubModule_EN.conf_en             = 1;
    tuningBuffer->Dpe_DVSSettings.SubModule_EN.occ_en              = 1;
    tuningBuffer->Dpe_DVSSettings.Iteration.y_IterTimes            = 1;
    tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_0    = 0;
    tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_1    = 1;
    tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_0    = 0;
    tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_1    = 1;

    __tuningMap[QUERY_KEY_PREVIEW] = tuningBuffer;
}

void
HW_DPE_DVS_CTRL_Tuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "===== DVS CTRL Parameters =====");
    for(auto &tuning : __tuningMap) {
        DPE_Config *tuningBuffer = tuning.second;
        logger
        .FastLogD("Scenario                                      %s", tuning.first.c_str())
        .FastLogD("Dpe_engineSelect                              %d", tuningBuffer->Dpe_engineSelect)
        .FastLogD("Dpe_is16BitMode                               %d", tuningBuffer->Dpe_is16BitMode)
        .FastLogD("Dpe_DVSSettings.mainEyeSel                    %d", tuningBuffer->Dpe_DVSSettings.mainEyeSel)
        .FastLogD("Dpe_DVSSettings.is_pd_mode                    %d", tuningBuffer->Dpe_DVSSettings.is_pd_mode)
        .FastLogD("Dpe_DVSSettings.pd_frame_num                  %d", tuningBuffer->Dpe_DVSSettings.pd_frame_num)
        .FastLogD("Dpe_DVSSettings.pd_st_x                       %d", tuningBuffer->Dpe_DVSSettings.pd_st_x)
        .FastLogD("Dpe_DVSSettings.frmWidth                      %d", tuningBuffer->Dpe_DVSSettings.frmWidth)
        .FastLogD("Dpe_DVSSettings.frmHeight                     %d", tuningBuffer->Dpe_DVSSettings.frmHeight)
        .FastLogD("Dpe_DVSSettings.L_engStart_x                  %d", tuningBuffer->Dpe_DVSSettings.L_engStart_x)
        .FastLogD("Dpe_DVSSettings.R_engStart_x                  %d", tuningBuffer->Dpe_DVSSettings.R_engStart_x)
        .FastLogD("Dpe_DVSSettings.engStart_y                    %d", tuningBuffer->Dpe_DVSSettings.engStart_y)
        .FastLogD("Dpe_DVSSettings.engWidth                      %d", tuningBuffer->Dpe_DVSSettings.engWidth)
        .FastLogD("Dpe_DVSSettings.engHeight                     %d", tuningBuffer->Dpe_DVSSettings.engHeight)
        .FastLogD("Dpe_DVSSettings.engStart_y                    %d", tuningBuffer->Dpe_DVSSettings.engStart_y)
        .FastLogD("Dpe_DVSSettings.occWidth                      %d", tuningBuffer->Dpe_DVSSettings.occWidth)
        .FastLogD("Dpe_DVSSettings.occStart_x                    %d", tuningBuffer->Dpe_DVSSettings.occStart_x)
        .FastLogD("Dpe_DVSSettings.pitch                         %d", tuningBuffer->Dpe_DVSSettings.pitch)
        .FastLogD("Dpe_DVSSettings.SubModule_EN.sbf_en           %d", tuningBuffer->Dpe_DVSSettings.SubModule_EN.sbf_en)
        .FastLogD("Dpe_DVSSettings.SubModule_EN.conf_en          %d", tuningBuffer->Dpe_DVSSettings.SubModule_EN.conf_en)
        .FastLogD("Dpe_DVSSettings.SubModule_EN.occ_en           %d", tuningBuffer->Dpe_DVSSettings.SubModule_EN.occ_en)
        .FastLogD("Dpe_DVSSettings.Iteration.y_IterTimes         %d", tuningBuffer->Dpe_DVSSettings.Iteration.y_IterTimes)
        .FastLogD("Dpe_DVSSettings.Iteration.y_IterStartDirect_0 %d", tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_0)
        .FastLogD("Dpe_DVSSettings.Iteration.y_IterStartDirect_1 %d", tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_1)
        .FastLogD("Dpe_DVSSettings.Iteration.x_IterStartDirect_0 %d", tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_0)
        .FastLogD("Dpe_DVSSettings.Iteration.x_IterStartDirect_1 %d", tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_1)
        .FastLogD("%s", "-------------------------------");
    }
    logger.print();
}

void
HW_DPE_DVS_CTRL_Tuning::_loadValuesFromDocument(const json& dpeValues)
{
    for(auto &tuning : dpeValues) {
        DPE_Config *tuningBuffer = NULL;
        std::string scenarioKey = tuning[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<std::string>();
        if(__tuningMap.find(scenarioKey) == __tuningMap.end()) {
            tuningBuffer = new DPE_Config();
            __tuningMap[scenarioKey] = tuningBuffer;
        } else {
            tuningBuffer = __tuningMap[scenarioKey];
        }

        ENUM_STEREO_SCENARIO scenario = eSTEREO_SCENARIO_PREVIEW;
        std::transform(scenarioKey.begin(), scenarioKey.end(), scenarioKey.begin(), ::tolower);
        if(std::string::npos != scenarioKey.find("cap")) {
            scenario = eSTEREO_SCENARIO_CAPTURE;
        }
        else if(std::string::npos != scenarioKey.find("record") ||
                std::string::npos != scenarioKey.find("video"))
        {
            scenario = eSTEREO_SCENARIO_RECORD;
        }

        const json &tuningValues = tuning[VALUE_KEY_VALUES];
        if(LogicalCamJSONUtil::HasMember(tuningValues, "mainEyeSel")) {
            tuningBuffer->Dpe_DVSSettings.mainEyeSel = static_cast<DPE_MAINEYE_SEL>(_getInt(tuningValues, "mainEyeSel"));
        } else {
            if(0 == StereoSettingProvider::getSensorRelativePosition()) {
                tuningBuffer->Dpe_DVSSettings.mainEyeSel = LEFT;
            } else {
                tuningBuffer->Dpe_DVSSettings.mainEyeSel = RIGHT;
            }
        }

        tuningBuffer->Dpe_engineSelect = static_cast<DPEMODE>(_getInt(tuningValues, "Dpe_engineSelect"));
        tuningBuffer->Dpe_is16BitMode  = _getInt(tuningValues, "Dpe_is16BitMode");
        tuningBuffer->Dpe_DVSSettings.is_pd_mode    = _getInt(tuningValues, "is_pd_mode");
        tuningBuffer->Dpe_DVSSettings.pd_frame_num  = _getInt(tuningValues, "pd_frame_num");

        _initFrameSize(&(tuningBuffer->Dpe_DVSSettings), scenario);
        _getValue(tuningValues, "pd_st_x",      tuningBuffer->Dpe_DVSSettings.pd_st_x);
        _getValue(tuningValues, "frmWidth",     tuningBuffer->Dpe_DVSSettings.frmWidth);
        _getValue(tuningValues, "frmHeight",    tuningBuffer->Dpe_DVSSettings.frmHeight);
        _getValue(tuningValues, "L_engStart_x", tuningBuffer->Dpe_DVSSettings.L_engStart_x);
        _getValue(tuningValues, "R_engStart_x", tuningBuffer->Dpe_DVSSettings.R_engStart_x);
        _getValue(tuningValues, "engStart_y",   tuningBuffer->Dpe_DVSSettings.engStart_y);
        _getValue(tuningValues, "engWidth",     tuningBuffer->Dpe_DVSSettings.engWidth);
        _getValue(tuningValues, "engHeight",    tuningBuffer->Dpe_DVSSettings.engHeight);
        _getValue(tuningValues, "occWidth",     tuningBuffer->Dpe_DVSSettings.occWidth);
        _getValue(tuningValues, "occStart_x",   tuningBuffer->Dpe_DVSSettings.occStart_x);
        _getValue(tuningValues, "pitch",        tuningBuffer->Dpe_DVSSettings.pitch);
        if(tuningBuffer->Dpe_DVSSettings.pitch == 0) {
            tuningBuffer->Dpe_DVSSettings.pitch = tuningBuffer->Dpe_DVSSettings.frmWidth;
        }

        tuningBuffer->Dpe_DVSSettings.SubModule_EN.sbf_en           = _getInt(tuningValues, "sbf_en");
        tuningBuffer->Dpe_DVSSettings.SubModule_EN.conf_en          = _getInt(tuningValues, "conf_en");
        tuningBuffer->Dpe_DVSSettings.SubModule_EN.occ_en           = _getInt(tuningValues, "occ_en");
        tuningBuffer->Dpe_DVSSettings.Iteration.y_IterTimes         = _getInt(tuningValues, "y_IterTimes");
        tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_0 = _getInt(tuningValues, "y_IterStartDirect_0");
        tuningBuffer->Dpe_DVSSettings.Iteration.y_IterStartDirect_1 = _getInt(tuningValues, "y_IterStartDirect_1");
        tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_0 = _getInt(tuningValues, "x_IterStartDirect_0");
        tuningBuffer->Dpe_DVSSettings.Iteration.x_IterStartDirect_1 = _getInt(tuningValues, "x_IterStartDirect_1");
    }
}
