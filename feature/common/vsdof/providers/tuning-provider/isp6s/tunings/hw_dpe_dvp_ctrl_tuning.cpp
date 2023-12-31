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
#define LOG_TAG "StereoTuningProvider_DVP_CTRL"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "hw_dpe_dvp_ctrl_tuning.h"
#include "DPE_Config.h"

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

HW_DPE_DVP_CTRL_Tuning::~HW_DPE_DVP_CTRL_Tuning()
{
    for(auto &t : __tuningMap) {
        if(t.second) {
            delete t.second;
        }
    }
}

bool
HW_DPE_DVP_CTRL_Tuning::retrieveTuningParams(TuningQuery_T &query)
{
    DPE_Config *dst = (DPE_Config *)query.results[STEREO_TUNING_NAME[E_TUNING_HW_DPE]];

    if(NULL == dst) {
        MY_LOGE("Cannot get target");
        return false;
    }

    DVP_Settings *src = NULL;
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

    ::memcpy(&(dst->Dpe_DVPSettings), src, sizeof(DVP_Settings));
    return true;
}

void
HW_DPE_DVP_CTRL_Tuning::_initFrameSize(DVP_Settings *setting, ENUM_STEREO_SCENARIO scenario)
{
    StereoArea area = StereoSizeProvider::getInstance()->getBufferSize(E_NOC, scenario, false);
    MSize contentSize = area.contentSize();
    setting->frmWidth    = contentSize.w;
    setting->frmHeight   = contentSize.h;
    setting->engWidth    = contentSize.w;
    setting->engHeight   = contentSize.h;
    setting->engStart_x  = area.startPt.x;
    setting->engStart_y  = area.startPt.y;
}

void
HW_DPE_DVP_CTRL_Tuning::_initDefaultValues()
{
    static_assert (sizeof(REG_DVP_CORE_CFG) == sizeof(DVP_CORE_CFG), "REG_DVP_CORE_CFG Size is incorrect");
    DVP_Settings *tuningBuffer = new DVP_Settings();

    if(0 == StereoSettingProvider::getSensorRelativePosition()) {
        tuningBuffer->mainEyeSel = LEFT;
    } else {
        tuningBuffer->mainEyeSel = RIGHT;
    }

    tuningBuffer->Y_only        = 0;
    tuningBuffer->disp_guide_en = 0;

    _initFrameSize(tuningBuffer, eSTEREO_SCENARIO_PREVIEW);

    tuningBuffer->SubModule_EN.asf_crm_en       = 1;
    tuningBuffer->SubModule_EN.asf_rm_en        = 1;
    tuningBuffer->SubModule_EN.asf_rd_en        = 1;
    tuningBuffer->SubModule_EN.asf_hf_en        = 1;
    tuningBuffer->SubModule_EN.wmf_hf_en        = 0;
    tuningBuffer->SubModule_EN.wmf_filt_en      = 0;
    tuningBuffer->SubModule_EN.asf_hf_rounds    = 2;
    tuningBuffer->SubModule_EN.asf_nb_rounds    = 2;
    tuningBuffer->SubModule_EN.wmf_filt_rounds  = 1;
    tuningBuffer->SubModule_EN.asf_recursive_en = 1;

    REG_DVP_CORE_CFG config;
    config.DVP_CORE_00.Bits.c_asf_thd_y      = 10;
    config.DVP_CORE_00.Bits.c_asf_thd_cbcr   = 6;
    config.DVP_CORE_00.Bits.c_asf_arm_ud     = 15;
    config.DVP_CORE_00.Bits.c_asf_arm_lr     = 31;
    config.DVP_CORE_01.Bits.c_rmv_mean_lut_0 = 8;
    config.DVP_CORE_01.Bits.c_rmv_mean_lut_1 = 8;
    config.DVP_CORE_01.Bits.c_rmv_mean_lut_2 = 8;
    config.DVP_CORE_01.Bits.c_rmv_mean_lut_3 = 8;
    config.DVP_CORE_02.Bits.c_rmv_mean_lut_4 = 8;
    config.DVP_CORE_02.Bits.c_rmv_mean_lut_5 = 4;
    config.DVP_CORE_02.Bits.c_rmv_mean_lut_6 = 4;
    config.DVP_CORE_02.Bits.c_rmv_mean_lut_7 = 4;
    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_0 = 10;
    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_1 = 10;
    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_2 = 10;
    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_3 = 10;
    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_4 = 10;
    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_5 = 16;
    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_6 = 16;
    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_7 = 16;
    config.DVP_CORE_05.Bits.c_hf_thd_r1      = 5;
    config.DVP_CORE_05.Bits.c_hf_thd_r2      = 1;
    config.DVP_CORE_05.Bits.c_hf_thd_r3      = 0;
    config.DVP_CORE_05.Bits.c_hf_thd_r4      = 0;
    config.DVP_CORE_06.Bits.c_hf_thd_r5      = 0;
    config.DVP_CORE_06.Bits.c_hf_thd_r6      = 0;
    config.DVP_CORE_06.Bits.c_hf_thd_r7      = 0;
    config.DVP_CORE_06.Bits.c_hf_thd_r8      = 0;
    config.DVP_CORE_07.Bits.c_hf_thin_en     = 1;
    config.DVP_CORE_07.Bits.c_hf_thd_thin    = 1;
    config.DVP_CORE_07.Bits.c_hf_thd_r9      = 0;
    config.DVP_CORE_07.Bits.c_hf_thd_r10     = 0;
    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_0  = 255;
    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_1  = 215;
    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_2  = 182;
    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_3  = 154;
    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_4  = 130;
    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_5  = 110;
    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_6  = 93;
    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_7  = 79;
    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_8  = 67;
    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_9  = 56;
    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_10 = 48;
    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_11 = 40;
    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_12 = 34;
    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_13 = 29;
    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_14 = 24;
    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_15 = 20;
    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_16 = 17;
    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_17 = 12;
    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_18 = 9;
    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_19 = 6;
    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_20 = 4;
    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_21 = 3;
    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_22 = 2;
    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_23 = 1;
    config.DVP_CORE_14.Bits.c_wmf_wgt_lut_24 = 1;
    config.DVP_CORE_15.Bits.c_hf_mode_r1     = 0;
    config.DVP_CORE_15.Bits.c_hf_mode_r2     = 0;
    config.DVP_CORE_15.Bits.c_hf_mode_r3     = 1;
    config.DVP_CORE_15.Bits.c_hf_mode_r4     = 1;
    config.DVP_CORE_15.Bits.c_hf_mode_r5     = 7;
    config.DVP_CORE_15.Bits.c_hf_mode_r6     = 7;
    config.DVP_CORE_15.Bits.c_hf_mode_r7     = 0;
    config.DVP_CORE_15.Bits.c_hf_mode_r8     = 0;
    config.DVP_CORE_15.Bits.c_hf_mode_r9     = 0;
    config.DVP_CORE_15.Bits.c_hf_mode_r10    = 0;
    ::memcpy(&(tuningBuffer->TuningBuf_CORE), &config, sizeof(REG_DVP_CORE_CFG));

    __tuningMap[QUERY_KEY_PREVIEW] = tuningBuffer;
}

void
HW_DPE_DVP_CTRL_Tuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "===== DVP CTRL Parameters =====");
    for(auto &tuning : __tuningMap) {
        DVP_Settings *tuningBuffer = tuning.second;
        REG_DVP_CORE_CFG config;
        ::memcpy(&config, &(tuningBuffer->TuningBuf_CORE), sizeof(REG_DVP_CORE_CFG));

        logger
        .FastLogD("Scenario             %s", tuning.first.c_str())
        .FastLogD("mainEyeSel           %d", tuningBuffer->mainEyeSel)
        .FastLogD("Y_only               %d", tuningBuffer->Y_only)
        .FastLogD("disp_guide_en        %d", tuningBuffer->disp_guide_en)
        .FastLogD("frmWidth             %d", tuningBuffer->frmWidth)
        .FastLogD("frmHeight            %d", tuningBuffer->frmHeight)
        .FastLogD("engStart_x           %d", tuningBuffer->engStart_x)
        .FastLogD("engStart_y           %d", tuningBuffer->engStart_y)
        .FastLogD("engWidth             %d", tuningBuffer->engWidth)
        .FastLogD("engHeight            %d", tuningBuffer->engHeight)
        .FastLogD("SubModule_EN.asf_crm_en       %d", tuningBuffer->SubModule_EN.asf_crm_en)
        .FastLogD("SubModule_EN.asf_rm_en        %d", tuningBuffer->SubModule_EN.asf_rm_en)
        .FastLogD("SubModule_EN.asf_rd_en        %d", tuningBuffer->SubModule_EN.asf_rd_en)
        .FastLogD("SubModule_EN.asf_hf_en        %d", tuningBuffer->SubModule_EN.asf_hf_en)
        .FastLogD("SubModule_EN.wmf_hf_en        %d", tuningBuffer->SubModule_EN.wmf_hf_en)
        .FastLogD("SubModule_EN.wmf_filt_en      %d", tuningBuffer->SubModule_EN.wmf_filt_en)
        .FastLogD("SubModule_EN.asf_hf_rounds    %d", tuningBuffer->SubModule_EN.asf_hf_rounds)
        .FastLogD("SubModule_EN.asf_nb_rounds    %d", tuningBuffer->SubModule_EN.asf_nb_rounds)
        .FastLogD("SubModule_EN.wmf_filt_rounds  %d", tuningBuffer->SubModule_EN.wmf_filt_rounds)
        .FastLogD("SubModule_EN.asf_recursive_en %d", tuningBuffer->SubModule_EN.asf_recursive_en)
        .FastLogD("c_asf_thd_y          %d", config.DVP_CORE_00.Bits.c_asf_thd_y)
        .FastLogD("c_asf_thd_cbcr       %d", config.DVP_CORE_00.Bits.c_asf_thd_cbcr)
        .FastLogD("c_asf_arm_ud         %d", config.DVP_CORE_00.Bits.c_asf_arm_ud)
        .FastLogD("c_asf_arm_lr         %d", config.DVP_CORE_00.Bits.c_asf_arm_lr)
        .FastLogD("c_rmv_mean_lut       % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d",
                    config.DVP_CORE_01.Bits.c_rmv_mean_lut_0,
                    config.DVP_CORE_01.Bits.c_rmv_mean_lut_1,
                    config.DVP_CORE_01.Bits.c_rmv_mean_lut_2,
                    config.DVP_CORE_01.Bits.c_rmv_mean_lut_3,
                    config.DVP_CORE_02.Bits.c_rmv_mean_lut_4,
                    config.DVP_CORE_02.Bits.c_rmv_mean_lut_5,
                    config.DVP_CORE_02.Bits.c_rmv_mean_lut_6,
                    config.DVP_CORE_02.Bits.c_rmv_mean_lut_7)
        .FastLogD("c_rmv_dsty_lut       % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d",
                    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_0,
                    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_1,
                    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_2,
                    config.DVP_CORE_03.Bits.c_rmv_dsty_lut_3,
                    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_4,
                    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_5,
                    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_6,
                    config.DVP_CORE_04.Bits.c_rmv_dsty_lut_7)
        .FastLogD("c_hf_thin_en         %d", config.DVP_CORE_07.Bits.c_hf_thin_en)
        .FastLogD("c_hf_thd_thin        %d", config.DVP_CORE_07.Bits.c_hf_thd_thin)
        .FastLogD("c_hf_thd             %d %d %d %d %d %d %d %d %d %d",
                    config.DVP_CORE_05.Bits.c_hf_thd_r1,
                    config.DVP_CORE_05.Bits.c_hf_thd_r2,
                    config.DVP_CORE_05.Bits.c_hf_thd_r3,
                    config.DVP_CORE_05.Bits.c_hf_thd_r4,
                    config.DVP_CORE_06.Bits.c_hf_thd_r5,
                    config.DVP_CORE_06.Bits.c_hf_thd_r6,
                    config.DVP_CORE_06.Bits.c_hf_thd_r7,
                    config.DVP_CORE_06.Bits.c_hf_thd_r8,
                    config.DVP_CORE_07.Bits.c_hf_thd_r9,
                    config.DVP_CORE_07.Bits.c_hf_thd_r10)
        .FastLogD("c_wmf_wgt_lut[00-07] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_0,
                    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_1,
                    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_2,
                    config.DVP_CORE_08.Bits.c_wmf_wgt_lut_3,
                    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_4,
                    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_5,
                    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_6,
                    config.DVP_CORE_09.Bits.c_wmf_wgt_lut_7)
        .FastLogD("c_wmf_wgt_lut[08-15] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_8,
                    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_9,
                    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_10,
                    config.DVP_CORE_10.Bits.c_wmf_wgt_lut_11,
                    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_12,
                    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_13,
                    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_14,
                    config.DVP_CORE_11.Bits.c_wmf_wgt_lut_15)
        .FastLogD("c_wmf_wgt_lut[16-24] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_16,
                    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_17,
                    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_18,
                    config.DVP_CORE_12.Bits.c_wmf_wgt_lut_19,
                    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_20,
                    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_21,
                    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_22,
                    config.DVP_CORE_13.Bits.c_wmf_wgt_lut_23,
                    config.DVP_CORE_14.Bits.c_wmf_wgt_lut_24)
        .FastLogD("c_hf_mode            %d %d %d %d %d %d %d %d %d %d",
                    config.DVP_CORE_15.Bits.c_hf_mode_r1,
                    config.DVP_CORE_15.Bits.c_hf_mode_r2,
                    config.DVP_CORE_15.Bits.c_hf_mode_r3,
                    config.DVP_CORE_15.Bits.c_hf_mode_r4,
                    config.DVP_CORE_15.Bits.c_hf_mode_r5,
                    config.DVP_CORE_15.Bits.c_hf_mode_r6,
                    config.DVP_CORE_15.Bits.c_hf_mode_r7,
                    config.DVP_CORE_15.Bits.c_hf_mode_r8,
                    config.DVP_CORE_15.Bits.c_hf_mode_r9,
                    config.DVP_CORE_15.Bits.c_hf_mode_r10)
        .FastLogD("%s", "-------------------------------");
    }
    logger.print();
}

void
HW_DPE_DVP_CTRL_Tuning::_loadValuesFromDocument(const json& dpeValues)
{
    for(auto &tuning : dpeValues) {
        DVP_Settings *tuningBuffer = NULL;
        std::string scenarioKey = tuning[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<std::string>();
        if(__tuningMap.find(scenarioKey) == __tuningMap.end()) {
            tuningBuffer = new DVP_Settings();
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
            tuningBuffer->mainEyeSel = static_cast<DPE_MAINEYE_SEL>(_getInt(tuningValues, "mainEyeSel"));
        } else {
            if(0 == StereoSettingProvider::getSensorRelativePosition()) {
                tuningBuffer->mainEyeSel = LEFT;
            } else {
                tuningBuffer->mainEyeSel = RIGHT;
            }
        }

        tuningBuffer->Y_only        = _getInt(tuningValues, "Y_only");
        tuningBuffer->disp_guide_en = _getInt(tuningValues, "disp_guide_en");

        tuningBuffer->SubModule_EN.asf_crm_en       = _getInt(tuningValues, "asf_crm_en");
        tuningBuffer->SubModule_EN.asf_rm_en        = _getInt(tuningValues, "asf_rm_en");
        tuningBuffer->SubModule_EN.asf_rd_en        = _getInt(tuningValues, "asf_rd_en");
        tuningBuffer->SubModule_EN.asf_hf_en        = _getInt(tuningValues, "asf_hf_en");
        tuningBuffer->SubModule_EN.wmf_hf_en        = _getInt(tuningValues, "wmf_hf_en");
        tuningBuffer->SubModule_EN.wmf_filt_en      = _getInt(tuningValues, "wmf_filt_en");
        tuningBuffer->SubModule_EN.asf_hf_rounds    = _getInt(tuningValues, "asf_hf_rounds");
        tuningBuffer->SubModule_EN.asf_nb_rounds    = _getInt(tuningValues, "asf_nb_rounds");
        tuningBuffer->SubModule_EN.wmf_filt_rounds  = _getInt(tuningValues, "wmf_filt_rounds");
        tuningBuffer->SubModule_EN.asf_recursive_en = _getInt(tuningValues, "asf_recursive_en");

        _initFrameSize(tuningBuffer, scenario);
        _getValue(tuningValues, "frmWidth",   tuningBuffer->frmWidth);
        _getValue(tuningValues, "frmHeight",  tuningBuffer->frmHeight);
        _getValue(tuningValues, "engStart_x", tuningBuffer->engStart_x);
        _getValue(tuningValues, "engStart_y", tuningBuffer->engStart_y);
        _getValue(tuningValues, "engWidth",   tuningBuffer->engWidth);
        _getValue(tuningValues, "engHeight",  tuningBuffer->engHeight);

        REG_DVP_CORE_CFG config;
        config.DVP_CORE_00.Bits.c_asf_thd_y    = _getInt(tuningValues, "c_asf_thd_y");
        config.DVP_CORE_00.Bits.c_asf_thd_cbcr = _getInt(tuningValues, "c_asf_thd_cbcr");
        config.DVP_CORE_00.Bits.c_asf_arm_ud   = _getInt(tuningValues, "c_asf_arm_ud");
        config.DVP_CORE_00.Bits.c_asf_arm_lr   = _getInt(tuningValues, "c_asf_arm_lr");
        if(_checkArrayAndSize(tuningValues, "c_rmv_mean_lut", 8))
        {
            const json &arrayValue = tuningValues["c_rmv_mean_lut"];
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_0 = arrayValue[0];
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_1 = arrayValue[1];
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_2 = arrayValue[2];
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_3 = arrayValue[3];
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_4 = arrayValue[4];
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_5 = arrayValue[5];
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_6 = arrayValue[6];
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_7 = arrayValue[7];
        } else {
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_0 = _getInt(tuningValues, "c_rmv_mean_lut_0");
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_1 = _getInt(tuningValues, "c_rmv_mean_lut_1");
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_2 = _getInt(tuningValues, "c_rmv_mean_lut_2");
            config.DVP_CORE_01.Bits.c_rmv_mean_lut_3 = _getInt(tuningValues, "c_rmv_mean_lut_3");
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_4 = _getInt(tuningValues, "c_rmv_mean_lut_4");
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_5 = _getInt(tuningValues, "c_rmv_mean_lut_5");
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_6 = _getInt(tuningValues, "c_rmv_mean_lut_6");
            config.DVP_CORE_02.Bits.c_rmv_mean_lut_7 = _getInt(tuningValues, "c_rmv_mean_lut_7");
        }

        if(_checkArrayAndSize(tuningValues, "c_rmv_dsty_lut", 8))
        {
            const json &arrayValue = tuningValues["c_rmv_dsty_lut"];
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_0 = arrayValue[0];
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_1 = arrayValue[1];
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_2 = arrayValue[2];
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_3 = arrayValue[3];
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_4 = arrayValue[4];
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_5 = arrayValue[5];
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_6 = arrayValue[6];
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_7 = arrayValue[7];
        } else {
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_0 = _getInt(tuningValues, "c_rmv_dsty_lut_0");
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_1 = _getInt(tuningValues, "c_rmv_dsty_lut_1");
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_2 = _getInt(tuningValues, "c_rmv_dsty_lut_2");
            config.DVP_CORE_03.Bits.c_rmv_dsty_lut_3 = _getInt(tuningValues, "c_rmv_dsty_lut_3");
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_4 = _getInt(tuningValues, "c_rmv_dsty_lut_4");
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_5 = _getInt(tuningValues, "c_rmv_dsty_lut_5");
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_6 = _getInt(tuningValues, "c_rmv_dsty_lut_6");
            config.DVP_CORE_04.Bits.c_rmv_dsty_lut_7 = _getInt(tuningValues, "c_rmv_dsty_lut_7");
        }

        if(_checkArrayAndSize(tuningValues, "c_hf_thd", 10))
        {
            const json &arrayValue = tuningValues["c_hf_thd"];
            config.DVP_CORE_05.Bits.c_hf_thd_r1  = arrayValue[0];
            config.DVP_CORE_05.Bits.c_hf_thd_r2  = arrayValue[1];
            config.DVP_CORE_05.Bits.c_hf_thd_r3  = arrayValue[2];
            config.DVP_CORE_05.Bits.c_hf_thd_r4  = arrayValue[3];
            config.DVP_CORE_06.Bits.c_hf_thd_r5  = arrayValue[4];
            config.DVP_CORE_06.Bits.c_hf_thd_r6  = arrayValue[5];
            config.DVP_CORE_06.Bits.c_hf_thd_r7  = arrayValue[6];
            config.DVP_CORE_06.Bits.c_hf_thd_r8  = arrayValue[7];
            config.DVP_CORE_07.Bits.c_hf_thd_r9  = arrayValue[8];
            config.DVP_CORE_07.Bits.c_hf_thd_r10 = arrayValue[9];
        } else {
            config.DVP_CORE_05.Bits.c_hf_thd_r1  = _getInt(tuningValues, "c_hf_thd_r1");
            config.DVP_CORE_05.Bits.c_hf_thd_r2  = _getInt(tuningValues, "c_hf_thd_r2");
            config.DVP_CORE_05.Bits.c_hf_thd_r3  = _getInt(tuningValues, "c_hf_thd_r3");
            config.DVP_CORE_05.Bits.c_hf_thd_r4  = _getInt(tuningValues, "c_hf_thd_r4");
            config.DVP_CORE_06.Bits.c_hf_thd_r5  = _getInt(tuningValues, "c_hf_thd_r5");
            config.DVP_CORE_06.Bits.c_hf_thd_r6  = _getInt(tuningValues, "c_hf_thd_r6");
            config.DVP_CORE_06.Bits.c_hf_thd_r7  = _getInt(tuningValues, "c_hf_thd_r7");
            config.DVP_CORE_06.Bits.c_hf_thd_r8  = _getInt(tuningValues, "c_hf_thd_r8");
            config.DVP_CORE_07.Bits.c_hf_thd_r9  = _getInt(tuningValues, "c_hf_thd_r9");
            config.DVP_CORE_07.Bits.c_hf_thd_r10 = _getInt(tuningValues, "c_hf_thd_r10");
        }

        config.DVP_CORE_07.Bits.c_hf_thin_en  = _getInt(tuningValues, "c_hf_thin_en");
        config.DVP_CORE_07.Bits.c_hf_thd_thin = _getInt(tuningValues, "c_hf_thd_thin");

        if(_checkArrayAndSize(tuningValues, "c_wmf_wgt_lut", 25))
        {
            const json &arrayValue = tuningValues["c_wmf_wgt_lut"];
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_0  = arrayValue[0];
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_1  = arrayValue[1];
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_2  = arrayValue[2];
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_3  = arrayValue[3];
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_4  = arrayValue[4];
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_5  = arrayValue[5];
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_6  = arrayValue[6];
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_7  = arrayValue[7];
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_8  = arrayValue[8];
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_9  = arrayValue[9];
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_10 = arrayValue[10];
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_11 = arrayValue[11];
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_12 = arrayValue[12];
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_13 = arrayValue[13];
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_14 = arrayValue[14];
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_15 = arrayValue[15];
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_16 = arrayValue[16];
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_17 = arrayValue[17];
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_18 = arrayValue[18];
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_19 = arrayValue[19];
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_20 = arrayValue[20];
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_21 = arrayValue[21];
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_22 = arrayValue[22];
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_23 = arrayValue[23];
            config.DVP_CORE_14.Bits.c_wmf_wgt_lut_24 = arrayValue[24];
        } else {
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_0  = _getInt(tuningValues, "c_wmf_wgt_lut_0");
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_1  = _getInt(tuningValues, "c_wmf_wgt_lut_1");
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_2  = _getInt(tuningValues, "c_wmf_wgt_lut_2");
            config.DVP_CORE_08.Bits.c_wmf_wgt_lut_3  = _getInt(tuningValues, "c_wmf_wgt_lut_3");
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_4  = _getInt(tuningValues, "c_wmf_wgt_lut_4");
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_5  = _getInt(tuningValues, "c_wmf_wgt_lut_5");
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_6  = _getInt(tuningValues, "c_wmf_wgt_lut_6");
            config.DVP_CORE_09.Bits.c_wmf_wgt_lut_7  = _getInt(tuningValues, "c_wmf_wgt_lut_7");
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_8  = _getInt(tuningValues, "c_wmf_wgt_lut_8");
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_9  = _getInt(tuningValues, "c_wmf_wgt_lut_9");
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_10 = _getInt(tuningValues, "c_wmf_wgt_lut_10");
            config.DVP_CORE_10.Bits.c_wmf_wgt_lut_11 = _getInt(tuningValues, "c_wmf_wgt_lut_11");
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_12 = _getInt(tuningValues, "c_wmf_wgt_lut_12");
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_13 = _getInt(tuningValues, "c_wmf_wgt_lut_13");
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_14 = _getInt(tuningValues, "c_wmf_wgt_lut_14");
            config.DVP_CORE_11.Bits.c_wmf_wgt_lut_15 = _getInt(tuningValues, "c_wmf_wgt_lut_15");
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_16 = _getInt(tuningValues, "c_wmf_wgt_lut_16");
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_17 = _getInt(tuningValues, "c_wmf_wgt_lut_17");
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_18 = _getInt(tuningValues, "c_wmf_wgt_lut_18");
            config.DVP_CORE_12.Bits.c_wmf_wgt_lut_19 = _getInt(tuningValues, "c_wmf_wgt_lut_19");
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_20 = _getInt(tuningValues, "c_wmf_wgt_lut_20");
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_21 = _getInt(tuningValues, "c_wmf_wgt_lut_21");
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_22 = _getInt(tuningValues, "c_wmf_wgt_lut_22");
            config.DVP_CORE_13.Bits.c_wmf_wgt_lut_23 = _getInt(tuningValues, "c_wmf_wgt_lut_23");
            config.DVP_CORE_14.Bits.c_wmf_wgt_lut_24 = _getInt(tuningValues, "c_wmf_wgt_lut_24");
        }

        if(_checkArrayAndSize(tuningValues, "c_hf_mode", 10))
        {
            const json &arrayValue = tuningValues["c_hf_mode"];
            config.DVP_CORE_15.Bits.c_hf_mode_r1  = arrayValue[0];
            config.DVP_CORE_15.Bits.c_hf_mode_r2  = arrayValue[1];
            config.DVP_CORE_15.Bits.c_hf_mode_r3  = arrayValue[2];
            config.DVP_CORE_15.Bits.c_hf_mode_r4  = arrayValue[3];
            config.DVP_CORE_15.Bits.c_hf_mode_r5  = arrayValue[4];
            config.DVP_CORE_15.Bits.c_hf_mode_r6  = arrayValue[5];
            config.DVP_CORE_15.Bits.c_hf_mode_r7  = arrayValue[6];
            config.DVP_CORE_15.Bits.c_hf_mode_r8  = arrayValue[7];
            config.DVP_CORE_15.Bits.c_hf_mode_r9  = arrayValue[8];
            config.DVP_CORE_15.Bits.c_hf_mode_r10 = arrayValue[9];
        } else {
            config.DVP_CORE_15.Bits.c_hf_mode_r1  = _getInt(tuningValues, "c_hf_mode_r1");
            config.DVP_CORE_15.Bits.c_hf_mode_r2  = _getInt(tuningValues, "c_hf_mode_r2");
            config.DVP_CORE_15.Bits.c_hf_mode_r3  = _getInt(tuningValues, "c_hf_mode_r3");
            config.DVP_CORE_15.Bits.c_hf_mode_r4  = _getInt(tuningValues, "c_hf_mode_r4");
            config.DVP_CORE_15.Bits.c_hf_mode_r5  = _getInt(tuningValues, "c_hf_mode_r5");
            config.DVP_CORE_15.Bits.c_hf_mode_r6  = _getInt(tuningValues, "c_hf_mode_r6");
            config.DVP_CORE_15.Bits.c_hf_mode_r7  = _getInt(tuningValues, "c_hf_mode_r7");
            config.DVP_CORE_15.Bits.c_hf_mode_r8  = _getInt(tuningValues, "c_hf_mode_r8");
            config.DVP_CORE_15.Bits.c_hf_mode_r9  = _getInt(tuningValues, "c_hf_mode_r9");
            config.DVP_CORE_15.Bits.c_hf_mode_r10 = _getInt(tuningValues, "c_hf_mode_r10");
        }

        ::memcpy(&(tuningBuffer->TuningBuf_CORE), &config, sizeof(REG_DVP_CORE_CFG));
    }
}
