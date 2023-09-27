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
#define LOG_TAG "StereoTuningProvider_DVS_ME"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "hw_dpe_dvs_me_tuning.h"

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

HW_DPE_DVS_ME_Tuning::~HW_DPE_DVS_ME_Tuning()
{
    for(auto &t : __tuningMap) {
        if(t.second) {
            delete t.second;
        }
    }
}

bool
HW_DPE_DVS_ME_Tuning::retrieveTuningParams(TuningQuery_T &query)
{
    DPE_Config *dst = (DPE_Config *)query.results[STEREO_TUNING_NAME[E_TUNING_HW_DPE]];

    if(NULL == dst) {
        MY_LOGE("Cannot get target");
        return false;
    }

    DVS_ME_CFG *src = NULL;
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
            (__tuningMap.find(QUERY_KEY_RECORD)  != __tuningMap.end() ||
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

    ::memcpy(&(dst->Dpe_DVSSettings.TuningBuf_ME), src, sizeof(REG_DVS_ME_CFG));

    return true;
}

void
HW_DPE_DVS_ME_Tuning::_initDefaultValues()
{
    static_assert (sizeof(REG_DVS_ME_CFG) == sizeof(DVS_ME_CFG), "REG_DVS_ME_CFG Size is incorrect");
    REG_DVS_ME_CFG config;
    config.DVS_ME_00.Bits.c_cand_num        =  48;
    config.DVS_ME_00.Bits.c_dv_start_pos    =   0;
    config.DVS_ME_00.Bits.c_cen_ws_mode     =   1;
    config.DVS_ME_00.Bits.c_cen_color_thd   =   0;
    config.DVS_ME_00.Bits.c_cen_avg_ws_mode =   0;
    config.DVS_ME_00.Bits.c_ad_weight       =   0;

    config.DVS_ME_01.Bits.c_lut_cen_c00     =   0;
    config.DVS_ME_01.Bits.c_lut_cen_c01     =  44;
    config.DVS_ME_01.Bits.c_lut_cen_c02     =  80;
    config.DVS_ME_01.Bits.c_lut_cen_c03     = 110;

    config.DVS_ME_02.Bits.c_lut_cen_c04     = 135;
    config.DVS_ME_02.Bits.c_lut_cen_c05     = 173;
    config.DVS_ME_02.Bits.c_lut_cen_c06     = 199;
    config.DVS_ME_02.Bits.c_lut_cen_c07     = 217;

    config.DVS_ME_03.Bits.c_lut_cen_c08     = 229;
    config.DVS_ME_03.Bits.c_lut_cen_c09     = 237;
    config.DVS_ME_03.Bits.c_lut_ad_c00      =   0;
    config.DVS_ME_03.Bits.c_lut_ad_c01      =  46;

    config.DVS_ME_04.Bits.c_lut_ad_c02      =  84;
    config.DVS_ME_04.Bits.c_lut_ad_c03      = 115;
    config.DVS_ME_04.Bits.c_lut_ad_c04      = 140;
    config.DVS_ME_04.Bits.c_lut_ad_c05      = 178;

    config.DVS_ME_05.Bits.c_lut_ad_c06      = 203;
    config.DVS_ME_05.Bits.c_lut_ad_c07      = 231;
    config.DVS_ME_05.Bits.c_lut_ad_c08      = 244;
    config.DVS_ME_05.Bits.c_lut_ad_c09      = 252;

    config.DVS_ME_06.Bits.c_lut_ad_c10      = 254;
    config.DVS_ME_06.Bits.c_lut_conf_c00    =  91;
    config.DVS_ME_06.Bits.c_lut_conf_c01    =  83;
    config.DVS_ME_06.Bits.c_lut_conf_c02    =  77;

    config.DVS_ME_07.Bits.c_lut_conf_c03    =  72;
    config.DVS_ME_07.Bits.c_lut_conf_c04    =  68;
    config.DVS_ME_07.Bits.c_lut_conf_c05    =  64;
    config.DVS_ME_07.Bits.c_lut_conf_c06    =  61;

    config.DVS_ME_08.Bits.c_recursive_mode  =   0;
    config.DVS_ME_08.Bits.c_rpd_en          =   1;
    config.DVS_ME_08.Bits.c_rpd_thd         =   4;
    config.DVS_ME_08.Bits.c_rpd_chk_ratio   =   4;

    config.DVS_ME_09.Bits.c_lut_p1_c00      =  80;
    config.DVS_ME_09.Bits.c_lut_p1_c01      =  80;
    config.DVS_ME_09.Bits.c_lut_p1_c02      =  80;

    config.DVS_ME_10.Bits.c_lut_p1_c03      =  50;
    config.DVS_ME_10.Bits.c_lut_p1_c04      =  20;
    config.DVS_ME_10.Bits.c_lut_p1_c05      =  20;

    config.DVS_ME_11.Bits.c_lut_p1_c06      =   8;
    config.DVS_ME_11.Bits.c_lut_p1_c07      =   8;
    config.DVS_ME_11.Bits.c_lut_p1_c08      =   8;

    config.DVS_ME_12.Bits.c_lut_p1_c09      =   8;
    config.DVS_ME_12.Bits.c_lut_p1_c10      =   8;
    config.DVS_ME_12.Bits.c_lut_p1_c11      =   8;

    config.DVS_ME_13.Bits.c_lut_p1_c12      =   8;
    config.DVS_ME_13.Bits.c_lut_p1_c13      =   8;
    config.DVS_ME_13.Bits.c_lut_p1_c14      =   8;

    config.DVS_ME_14.Bits.c_lut_p2_c00      = 300;
    config.DVS_ME_14.Bits.c_lut_p2_c01      = 300;
    config.DVS_ME_14.Bits.c_lut_p2_c02      = 300;

    config.DVS_ME_15.Bits.c_lut_p2_c03      = 187;
    config.DVS_ME_15.Bits.c_lut_p2_c04      =  75;
    config.DVS_ME_15.Bits.c_lut_p2_c05      =  75;

    config.DVS_ME_16.Bits.c_lut_p2_c06      =  30;
    config.DVS_ME_16.Bits.c_lut_p2_c07      =  30;
    config.DVS_ME_16.Bits.c_lut_p2_c08      =  30;

    config.DVS_ME_17.Bits.c_lut_p2_c09      =  30;
    config.DVS_ME_17.Bits.c_lut_p2_c10      =  30;
    config.DVS_ME_17.Bits.c_lut_p2_c11      =  30;

    config.DVS_ME_18.Bits.c_lut_p2_c12      =  30;
    config.DVS_ME_18.Bits.c_lut_p2_c13      =  30;
    config.DVS_ME_18.Bits.c_lut_p2_c14      =  30;

    config.DVS_ME_19.Bits.c_lut_p3_c00      =   0;
    config.DVS_ME_19.Bits.c_lut_p3_c01      =  10;
    config.DVS_ME_19.Bits.c_lut_p3_c02      =  20;

    config.DVS_ME_20.Bits.c_lut_p3_c03      = 120;
    config.DVS_ME_20.Bits.c_lut_p3_c04      = 120;
    config.DVS_ME_20.Bits.c_lut_p3_c05      = 120;

    config.DVS_ME_21.Bits.c_lut_p3_c06      = 120;
    config.DVS_ME_21.Bits.c_lut_p3_c07      = 120;

    config.DVS_ME_22.Bits.c_wgt_dir_0       =   8;
    config.DVS_ME_22.Bits.c_wgt_dir_1_3     =   8;
    config.DVS_ME_22.Bits.c_wgt_dir_2       =   8;
    config.DVS_ME_22.Bits.c_wgt_dir_4       =   8;

    config.DVS_ME_23.Bits.c_fractional_cv   =   0;
    config.DVS_ME_23.Bits.c_subpixel_ip     =   4;

    config.DVS_ME_24.Bits.c_conf_dv_diff    =   1;
    config.DVS_ME_24.Bits.c_bypass_p3_thd   =   1;
    config.DVS_ME_24.Bits.c_p3_by_conf_rpd  =   3;
    config.DVS_ME_24.Bits.c_p3_conf_thd     =   0;
    config.DVS_ME_24.Bits.c_conf_cost_diff  =   0;

    DVS_ME_CFG *tuningBuffer = new DVS_ME_CFG();
    ::memcpy(tuningBuffer, &config, sizeof(REG_DVS_ME_CFG));
    __tuningMap[QUERY_KEY_PREVIEW] = tuningBuffer;
}

void
HW_DPE_DVS_ME_Tuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "===== DVS ME Parameters =====");
    for(auto &tuning : __tuningMap) {
        DVS_ME_CFG *tuningBuffer = tuning.second;
        REG_DVS_ME_CFG config;
        ::memcpy(&config, tuningBuffer, sizeof(REG_DVS_ME_CFG));
        logger
        .FastLogD("Scenario                   %s", tuning.first.c_str())
        .FastLogD("c_cand_num                 %d", config.DVS_ME_00.Bits.c_cand_num)
        .FastLogD("c_dv_start_pos             %d", config.DVS_ME_00.Bits.c_dv_start_pos)
        .FastLogD("c_cen_ws_mode              %d", config.DVS_ME_00.Bits.c_cen_ws_mode)
        .FastLogD("c_cen_color_thd            %d", config.DVS_ME_00.Bits.c_cen_color_thd)
        .FastLogD("c_cen_avg_ws_mode          %d", config.DVS_ME_00.Bits.c_cen_avg_ws_mode)
        .FastLogD("c_ad_weight                %d", config.DVS_ME_00.Bits.c_ad_weight)
        .FastLogD("c_lut_cen_c00-09           % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d",
                config.DVS_ME_01.Bits.c_lut_cen_c00,
                config.DVS_ME_01.Bits.c_lut_cen_c01,
                config.DVS_ME_01.Bits.c_lut_cen_c02,
                config.DVS_ME_01.Bits.c_lut_cen_c03,
                config.DVS_ME_02.Bits.c_lut_cen_c04,
                config.DVS_ME_02.Bits.c_lut_cen_c05,
                config.DVS_ME_02.Bits.c_lut_cen_c06,
                config.DVS_ME_02.Bits.c_lut_cen_c07,
                config.DVS_ME_03.Bits.c_lut_cen_c08,
                config.DVS_ME_03.Bits.c_lut_cen_c09)
        .FastLogD("c_lut_ad_c00-10            % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d",
                config.DVS_ME_03.Bits.c_lut_ad_c00,
                config.DVS_ME_03.Bits.c_lut_ad_c01,
                config.DVS_ME_04.Bits.c_lut_ad_c02,
                config.DVS_ME_04.Bits.c_lut_ad_c03,
                config.DVS_ME_04.Bits.c_lut_ad_c04,
                config.DVS_ME_04.Bits.c_lut_ad_c05,
                config.DVS_ME_05.Bits.c_lut_ad_c06,
                config.DVS_ME_05.Bits.c_lut_ad_c07,
                config.DVS_ME_05.Bits.c_lut_ad_c08,
                config.DVS_ME_05.Bits.c_lut_ad_c09,
                config.DVS_ME_06.Bits.c_lut_ad_c10)
        .FastLogD("c_lut_conf_c00-06          % 3d % 3d % 3d % 3d % 3d % 3d % 3d",
                config.DVS_ME_06.Bits.c_lut_conf_c00,
                config.DVS_ME_06.Bits.c_lut_conf_c01,
                config.DVS_ME_06.Bits.c_lut_conf_c02,
                config.DVS_ME_07.Bits.c_lut_conf_c03,
                config.DVS_ME_07.Bits.c_lut_conf_c04,
                config.DVS_ME_07.Bits.c_lut_conf_c05,
                config.DVS_ME_07.Bits.c_lut_conf_c06)
        .FastLogD("c_recursive_mode           %d", config.DVS_ME_08.Bits.c_recursive_mode)
        .FastLogD("c_rpd_en                   %d", config.DVS_ME_08.Bits.c_rpd_en)
        .FastLogD("c_rpd_thd                  %d", config.DVS_ME_08.Bits.c_rpd_thd)
        .FastLogD("c_rpd_chk_ratio            %d", config.DVS_ME_08.Bits.c_rpd_chk_ratio)
        .FastLogD("c_lut_p1_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                config.DVS_ME_09.Bits.c_lut_p1_c00,
                config.DVS_ME_09.Bits.c_lut_p1_c01,
                config.DVS_ME_09.Bits.c_lut_p1_c02,
                config.DVS_ME_10.Bits.c_lut_p1_c03,
                config.DVS_ME_10.Bits.c_lut_p1_c04,
                config.DVS_ME_10.Bits.c_lut_p1_c05,
                config.DVS_ME_11.Bits.c_lut_p1_c06,
                config.DVS_ME_11.Bits.c_lut_p1_c07)
        .FastLogD("c_lut_p1_c08-14            % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                config.DVS_ME_11.Bits.c_lut_p1_c08,
                config.DVS_ME_12.Bits.c_lut_p1_c09,
                config.DVS_ME_12.Bits.c_lut_p1_c10,
                config.DVS_ME_12.Bits.c_lut_p1_c11,
                config.DVS_ME_13.Bits.c_lut_p1_c12,
                config.DVS_ME_13.Bits.c_lut_p1_c13,
                config.DVS_ME_13.Bits.c_lut_p1_c14)
        .FastLogD("c_lut_p2_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                config.DVS_ME_14.Bits.c_lut_p2_c00,
                config.DVS_ME_14.Bits.c_lut_p2_c01,
                config.DVS_ME_14.Bits.c_lut_p2_c02,
                config.DVS_ME_15.Bits.c_lut_p2_c03,
                config.DVS_ME_15.Bits.c_lut_p2_c04,
                config.DVS_ME_15.Bits.c_lut_p2_c05,
                config.DVS_ME_16.Bits.c_lut_p2_c06,
                config.DVS_ME_16.Bits.c_lut_p2_c07)
        .FastLogD("c_lut_p2_c08-14            % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                config.DVS_ME_16.Bits.c_lut_p2_c08,
                config.DVS_ME_17.Bits.c_lut_p2_c09,
                config.DVS_ME_17.Bits.c_lut_p2_c10,
                config.DVS_ME_17.Bits.c_lut_p2_c11,
                config.DVS_ME_18.Bits.c_lut_p2_c12,
                config.DVS_ME_18.Bits.c_lut_p2_c13,
                config.DVS_ME_18.Bits.c_lut_p2_c14)
        .FastLogD("c_lut_p3_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d",
                config.DVS_ME_19.Bits.c_lut_p3_c00,
                config.DVS_ME_19.Bits.c_lut_p3_c01,
                config.DVS_ME_19.Bits.c_lut_p3_c02,
                config.DVS_ME_20.Bits.c_lut_p3_c03,
                config.DVS_ME_20.Bits.c_lut_p3_c04,
                config.DVS_ME_20.Bits.c_lut_p3_c05,
                config.DVS_ME_21.Bits.c_lut_p3_c06,
                config.DVS_ME_21.Bits.c_lut_p3_c07)
        .FastLogD("c_wgt_dir_0               %d", config.DVS_ME_22.Bits.c_wgt_dir_0)
        .FastLogD("c_wgt_dir_1_3             %d", config.DVS_ME_22.Bits.c_wgt_dir_1_3)
        .FastLogD("c_wgt_dir_2               %d", config.DVS_ME_22.Bits.c_wgt_dir_2)
        .FastLogD("c_wgt_dir_4               %d", config.DVS_ME_22.Bits.c_wgt_dir_4)
        .FastLogD("c_fractional_cv           %d", config.DVS_ME_23.Bits.c_fractional_cv)
        .FastLogD("c_subpixel_ip             %d", config.DVS_ME_23.Bits.c_subpixel_ip)
        .FastLogD("c_conf_dv_diff            %d", config.DVS_ME_24.Bits.c_conf_dv_diff)
        .FastLogD("c_bypass_p3_thd           %d", config.DVS_ME_24.Bits.c_bypass_p3_thd)
        .FastLogD("c_p3_by_conf_rpd          %d", config.DVS_ME_24.Bits.c_p3_by_conf_rpd)
        .FastLogD("c_p3_conf_thd             %d", config.DVS_ME_24.Bits.c_p3_conf_thd)
        .FastLogD("c_conf_cost_diff          %d", config.DVS_ME_24.Bits.c_conf_cost_diff)
        .FastLogD("%s", "-----------------------------");
    }
    logger.print();
}

void
HW_DPE_DVS_ME_Tuning::_loadValuesFromDocument(const json& dpeValues)
{
    for(auto &tuning : dpeValues) {
        DVS_ME_CFG *tuningBuffer = NULL;
        std::string scenarioKey = tuning[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<std::string>();
        if(__tuningMap.find(scenarioKey) == __tuningMap.end()) {
            tuningBuffer = new DVS_ME_CFG();
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
        REG_DVS_ME_CFG config;
        config.DVS_ME_00.Bits.c_cand_num        = _getInt(tuningValues, "c_cand_num");
        config.DVS_ME_00.Bits.c_dv_start_pos    = _getInt(tuningValues, "c_dv_start_pos");
        config.DVS_ME_00.Bits.c_cen_ws_mode     = _getInt(tuningValues, "c_cen_ws_mode");
        config.DVS_ME_00.Bits.c_cen_color_thd   = _getInt(tuningValues, "c_cen_color_thd");
        config.DVS_ME_00.Bits.c_cen_avg_ws_mode = _getInt(tuningValues, "c_cen_avg_ws_mode");
        config.DVS_ME_00.Bits.c_ad_weight       = _getInt(tuningValues, "c_ad_weight");
        if(_checkArrayAndSize(tuningValues, "c_lut_cen", 10)) {
            const json &arrayValue = tuningValues["c_lut_cen"];
            config.DVS_ME_01.Bits.c_lut_cen_c00 = arrayValue[0];
            config.DVS_ME_01.Bits.c_lut_cen_c01 = arrayValue[1];
            config.DVS_ME_01.Bits.c_lut_cen_c02 = arrayValue[2];
            config.DVS_ME_01.Bits.c_lut_cen_c03 = arrayValue[3];
            config.DVS_ME_02.Bits.c_lut_cen_c04 = arrayValue[4];
            config.DVS_ME_02.Bits.c_lut_cen_c05 = arrayValue[5];
            config.DVS_ME_02.Bits.c_lut_cen_c06 = arrayValue[6];
            config.DVS_ME_02.Bits.c_lut_cen_c07 = arrayValue[7];
            config.DVS_ME_03.Bits.c_lut_cen_c08 = arrayValue[8];
            config.DVS_ME_03.Bits.c_lut_cen_c09 = arrayValue[9];
        } else {
            config.DVS_ME_01.Bits.c_lut_cen_c00 = _getInt(tuningValues, "c_lut_cen_c00");
            config.DVS_ME_01.Bits.c_lut_cen_c01 = _getInt(tuningValues, "c_lut_cen_c01");
            config.DVS_ME_01.Bits.c_lut_cen_c02 = _getInt(tuningValues, "c_lut_cen_c02");
            config.DVS_ME_01.Bits.c_lut_cen_c03 = _getInt(tuningValues, "c_lut_cen_c03");
            config.DVS_ME_02.Bits.c_lut_cen_c04 = _getInt(tuningValues, "c_lut_cen_c04");
            config.DVS_ME_02.Bits.c_lut_cen_c05 = _getInt(tuningValues, "c_lut_cen_c05");
            config.DVS_ME_02.Bits.c_lut_cen_c06 = _getInt(tuningValues, "c_lut_cen_c06");
            config.DVS_ME_02.Bits.c_lut_cen_c07 = _getInt(tuningValues, "c_lut_cen_c07");
            config.DVS_ME_03.Bits.c_lut_cen_c08 = _getInt(tuningValues, "c_lut_cen_c08");
            config.DVS_ME_03.Bits.c_lut_cen_c09 = _getInt(tuningValues, "c_lut_cen_c09");
        }

        if(_checkArrayAndSize(tuningValues, "c_lut_ad", 11)) {
            const json &arrayValue = tuningValues["c_lut_ad"];
            config.DVS_ME_03.Bits.c_lut_ad_c00 = arrayValue[0];
            config.DVS_ME_03.Bits.c_lut_ad_c01 = arrayValue[1];
            config.DVS_ME_04.Bits.c_lut_ad_c02 = arrayValue[2];
            config.DVS_ME_04.Bits.c_lut_ad_c03 = arrayValue[3];
            config.DVS_ME_04.Bits.c_lut_ad_c04 = arrayValue[4];
            config.DVS_ME_04.Bits.c_lut_ad_c05 = arrayValue[5];
            config.DVS_ME_05.Bits.c_lut_ad_c06 = arrayValue[6];
            config.DVS_ME_05.Bits.c_lut_ad_c07 = arrayValue[7];
            config.DVS_ME_05.Bits.c_lut_ad_c08 = arrayValue[8];
            config.DVS_ME_05.Bits.c_lut_ad_c09 = arrayValue[9];
            config.DVS_ME_06.Bits.c_lut_ad_c10 = arrayValue[10];
        } else {
            config.DVS_ME_03.Bits.c_lut_ad_c00 = _getInt(tuningValues, "c_lut_ad_c00");
            config.DVS_ME_03.Bits.c_lut_ad_c01 = _getInt(tuningValues, "c_lut_ad_c01");
            config.DVS_ME_04.Bits.c_lut_ad_c02 = _getInt(tuningValues, "c_lut_ad_c02");
            config.DVS_ME_04.Bits.c_lut_ad_c03 = _getInt(tuningValues, "c_lut_ad_c03");
            config.DVS_ME_04.Bits.c_lut_ad_c04 = _getInt(tuningValues, "c_lut_ad_c04");
            config.DVS_ME_04.Bits.c_lut_ad_c05 = _getInt(tuningValues, "c_lut_ad_c05");
            config.DVS_ME_05.Bits.c_lut_ad_c06 = _getInt(tuningValues, "c_lut_ad_c06");
            config.DVS_ME_05.Bits.c_lut_ad_c07 = _getInt(tuningValues, "c_lut_ad_c07");
            config.DVS_ME_05.Bits.c_lut_ad_c08 = _getInt(tuningValues, "c_lut_ad_c08");
            config.DVS_ME_05.Bits.c_lut_ad_c09 = _getInt(tuningValues, "c_lut_ad_c09");
            config.DVS_ME_06.Bits.c_lut_ad_c10 = _getInt(tuningValues, "c_lut_ad_c10");
        }

        if(_checkArrayAndSize(tuningValues, "c_lut_conf", 7)) {
            const json &arrayValue = tuningValues["c_lut_conf"];
            config.DVS_ME_06.Bits.c_lut_conf_c00 = arrayValue[0];
            config.DVS_ME_06.Bits.c_lut_conf_c01 = arrayValue[1];
            config.DVS_ME_06.Bits.c_lut_conf_c02 = arrayValue[2];
            config.DVS_ME_07.Bits.c_lut_conf_c03 = arrayValue[3];
            config.DVS_ME_07.Bits.c_lut_conf_c04 = arrayValue[4];
            config.DVS_ME_07.Bits.c_lut_conf_c05 = arrayValue[5];
            config.DVS_ME_07.Bits.c_lut_conf_c06 = arrayValue[6];
        } else {
            config.DVS_ME_06.Bits.c_lut_conf_c00 = _getInt(tuningValues, "c_lut_conf_c00");
            config.DVS_ME_06.Bits.c_lut_conf_c01 = _getInt(tuningValues, "c_lut_conf_c01");
            config.DVS_ME_06.Bits.c_lut_conf_c02 = _getInt(tuningValues, "c_lut_conf_c02");
            config.DVS_ME_07.Bits.c_lut_conf_c03 = _getInt(tuningValues, "c_lut_conf_c03");
            config.DVS_ME_07.Bits.c_lut_conf_c04 = _getInt(tuningValues, "c_lut_conf_c04");
            config.DVS_ME_07.Bits.c_lut_conf_c05 = _getInt(tuningValues, "c_lut_conf_c05");
            config.DVS_ME_07.Bits.c_lut_conf_c06 = _getInt(tuningValues, "c_lut_conf_c06");
        }

        config.DVS_ME_08.Bits.c_recursive_mode = _getInt(tuningValues, "c_recursive_mode");
        config.DVS_ME_08.Bits.c_rpd_en         = _getInt(tuningValues, "c_rpd_en");
        config.DVS_ME_08.Bits.c_rpd_thd        = _getInt(tuningValues, "c_rpd_thd");
        config.DVS_ME_08.Bits.c_rpd_chk_ratio  = _getInt(tuningValues, "c_rpd_chk_ratio");

        if(_checkArrayAndSize(tuningValues, "c_lut_p1", 15)) {
            const json &arrayValue = tuningValues["c_lut_p1"];
            config.DVS_ME_09.Bits.c_lut_p1_c00 = arrayValue[0];
            config.DVS_ME_09.Bits.c_lut_p1_c01 = arrayValue[1];
            config.DVS_ME_09.Bits.c_lut_p1_c02 = arrayValue[2];
            config.DVS_ME_10.Bits.c_lut_p1_c03 = arrayValue[3];
            config.DVS_ME_10.Bits.c_lut_p1_c04 = arrayValue[4];
            config.DVS_ME_10.Bits.c_lut_p1_c05 = arrayValue[5];
            config.DVS_ME_11.Bits.c_lut_p1_c06 = arrayValue[6];
            config.DVS_ME_11.Bits.c_lut_p1_c07 = arrayValue[7];
            config.DVS_ME_11.Bits.c_lut_p1_c08 = arrayValue[8];
            config.DVS_ME_12.Bits.c_lut_p1_c09 = arrayValue[9];
            config.DVS_ME_12.Bits.c_lut_p1_c10 = arrayValue[10];
            config.DVS_ME_12.Bits.c_lut_p1_c11 = arrayValue[11];
            config.DVS_ME_13.Bits.c_lut_p1_c12 = arrayValue[12];
            config.DVS_ME_13.Bits.c_lut_p1_c13 = arrayValue[13];
            config.DVS_ME_13.Bits.c_lut_p1_c14 = arrayValue[14];
        } else {
            config.DVS_ME_09.Bits.c_lut_p1_c00 = _getInt(tuningValues, "c_lut_p1_c00");
            config.DVS_ME_09.Bits.c_lut_p1_c01 = _getInt(tuningValues, "c_lut_p1_c01");
            config.DVS_ME_09.Bits.c_lut_p1_c02 = _getInt(tuningValues, "c_lut_p1_c02");
            config.DVS_ME_10.Bits.c_lut_p1_c03 = _getInt(tuningValues, "c_lut_p1_c03");
            config.DVS_ME_10.Bits.c_lut_p1_c04 = _getInt(tuningValues, "c_lut_p1_c04");
            config.DVS_ME_10.Bits.c_lut_p1_c05 = _getInt(tuningValues, "c_lut_p1_c05");
            config.DVS_ME_11.Bits.c_lut_p1_c06 = _getInt(tuningValues, "c_lut_p1_c06");
            config.DVS_ME_11.Bits.c_lut_p1_c07 = _getInt(tuningValues, "c_lut_p1_c07");
            config.DVS_ME_11.Bits.c_lut_p1_c08 = _getInt(tuningValues, "c_lut_p1_c08");
            config.DVS_ME_12.Bits.c_lut_p1_c09 = _getInt(tuningValues, "c_lut_p1_c09");
            config.DVS_ME_12.Bits.c_lut_p1_c10 = _getInt(tuningValues, "c_lut_p1_c10");
            config.DVS_ME_12.Bits.c_lut_p1_c11 = _getInt(tuningValues, "c_lut_p1_c11");
            config.DVS_ME_13.Bits.c_lut_p1_c12 = _getInt(tuningValues, "c_lut_p1_c12");
            config.DVS_ME_13.Bits.c_lut_p1_c13 = _getInt(tuningValues, "c_lut_p1_c13");
            config.DVS_ME_13.Bits.c_lut_p1_c14 = _getInt(tuningValues, "c_lut_p1_c14");
        }

        if(_checkArrayAndSize(tuningValues, "c_lut_p2", 15)) {
            const json &arrayValue = tuningValues["c_lut_p2"];
            config.DVS_ME_14.Bits.c_lut_p2_c00 = arrayValue[0];
            config.DVS_ME_14.Bits.c_lut_p2_c01 = arrayValue[1];
            config.DVS_ME_14.Bits.c_lut_p2_c02 = arrayValue[2];
            config.DVS_ME_15.Bits.c_lut_p2_c03 = arrayValue[3];
            config.DVS_ME_15.Bits.c_lut_p2_c04 = arrayValue[4];
            config.DVS_ME_15.Bits.c_lut_p2_c05 = arrayValue[5];
            config.DVS_ME_16.Bits.c_lut_p2_c06 = arrayValue[6];
            config.DVS_ME_16.Bits.c_lut_p2_c07 = arrayValue[7];
            config.DVS_ME_16.Bits.c_lut_p2_c08 = arrayValue[8];
            config.DVS_ME_17.Bits.c_lut_p2_c09 = arrayValue[9];
            config.DVS_ME_17.Bits.c_lut_p2_c10 = arrayValue[10];
            config.DVS_ME_17.Bits.c_lut_p2_c11 = arrayValue[11];
            config.DVS_ME_18.Bits.c_lut_p2_c12 = arrayValue[12];
            config.DVS_ME_18.Bits.c_lut_p2_c13 = arrayValue[13];
            config.DVS_ME_18.Bits.c_lut_p2_c14 = arrayValue[14];
        } else {
            config.DVS_ME_14.Bits.c_lut_p2_c00 = _getInt(tuningValues, "c_lut_p2_c00");
            config.DVS_ME_14.Bits.c_lut_p2_c01 = _getInt(tuningValues, "c_lut_p2_c01");
            config.DVS_ME_14.Bits.c_lut_p2_c02 = _getInt(tuningValues, "c_lut_p2_c02");
            config.DVS_ME_15.Bits.c_lut_p2_c03 = _getInt(tuningValues, "c_lut_p2_c03");
            config.DVS_ME_15.Bits.c_lut_p2_c04 = _getInt(tuningValues, "c_lut_p2_c04");
            config.DVS_ME_15.Bits.c_lut_p2_c05 = _getInt(tuningValues, "c_lut_p2_c05");
            config.DVS_ME_16.Bits.c_lut_p2_c06 = _getInt(tuningValues, "c_lut_p2_c06");
            config.DVS_ME_16.Bits.c_lut_p2_c07 = _getInt(tuningValues, "c_lut_p2_c07");
            config.DVS_ME_16.Bits.c_lut_p2_c08 = _getInt(tuningValues, "c_lut_p2_c08");
            config.DVS_ME_17.Bits.c_lut_p2_c09 = _getInt(tuningValues, "c_lut_p2_c09");
            config.DVS_ME_17.Bits.c_lut_p2_c10 = _getInt(tuningValues, "c_lut_p2_c10");
            config.DVS_ME_17.Bits.c_lut_p2_c11 = _getInt(tuningValues, "c_lut_p2_c11");
            config.DVS_ME_18.Bits.c_lut_p2_c12 = _getInt(tuningValues, "c_lut_p2_c12");
            config.DVS_ME_18.Bits.c_lut_p2_c13 = _getInt(tuningValues, "c_lut_p2_c13");
            config.DVS_ME_18.Bits.c_lut_p2_c14 = _getInt(tuningValues, "c_lut_p2_c14");
        }

        if(_checkArrayAndSize(tuningValues, "c_lut_p3", 8)) {
            const json &arrayValue = tuningValues["c_lut_p3"];
            config.DVS_ME_19.Bits.c_lut_p3_c00 = arrayValue[0];
            config.DVS_ME_19.Bits.c_lut_p3_c01 = arrayValue[1];
            config.DVS_ME_19.Bits.c_lut_p3_c02 = arrayValue[2];
            config.DVS_ME_20.Bits.c_lut_p3_c03 = arrayValue[3];
            config.DVS_ME_20.Bits.c_lut_p3_c04 = arrayValue[4];
            config.DVS_ME_20.Bits.c_lut_p3_c05 = arrayValue[5];
            config.DVS_ME_21.Bits.c_lut_p3_c06 = arrayValue[6];
            config.DVS_ME_21.Bits.c_lut_p3_c07 = arrayValue[7];
        } else {
            config.DVS_ME_19.Bits.c_lut_p3_c00 = _getInt(tuningValues, "c_lut_p3_c00");
            config.DVS_ME_19.Bits.c_lut_p3_c01 = _getInt(tuningValues, "c_lut_p3_c01");
            config.DVS_ME_19.Bits.c_lut_p3_c02 = _getInt(tuningValues, "c_lut_p3_c02");
            config.DVS_ME_20.Bits.c_lut_p3_c03 = _getInt(tuningValues, "c_lut_p3_c03");
            config.DVS_ME_20.Bits.c_lut_p3_c04 = _getInt(tuningValues, "c_lut_p3_c04");
            config.DVS_ME_20.Bits.c_lut_p3_c05 = _getInt(tuningValues, "c_lut_p3_c05");
            config.DVS_ME_21.Bits.c_lut_p3_c06 = _getInt(tuningValues, "c_lut_p3_c06");
            config.DVS_ME_21.Bits.c_lut_p3_c07 = _getInt(tuningValues, "c_lut_p3_c07");
        }

        config.DVS_ME_22.Bits.c_wgt_dir_0      = _getInt(tuningValues, "c_wgt_dir_0");
        config.DVS_ME_22.Bits.c_wgt_dir_1_3    = _getInt(tuningValues, "c_wgt_dir_1_3");
        config.DVS_ME_22.Bits.c_wgt_dir_2      = _getInt(tuningValues, "c_wgt_dir_2");
        config.DVS_ME_22.Bits.c_wgt_dir_4      = _getInt(tuningValues, "c_wgt_dir_4");
        config.DVS_ME_23.Bits.c_fractional_cv  = _getInt(tuningValues, "c_fractional_cv");
        config.DVS_ME_23.Bits.c_subpixel_ip    = _getInt(tuningValues, "c_subpixel_ip");
        config.DVS_ME_24.Bits.c_conf_dv_diff   = _getInt(tuningValues, "c_conf_dv_diff");
        config.DVS_ME_24.Bits.c_bypass_p3_thd  = _getInt(tuningValues, "c_bypass_p3_thd");
        config.DVS_ME_24.Bits.c_p3_by_conf_rpd = _getInt(tuningValues, "c_p3_by_conf_rpd");
        config.DVS_ME_24.Bits.c_p3_conf_thd    = _getInt(tuningValues, "c_p3_conf_thd");
        config.DVS_ME_24.Bits.c_conf_cost_diff = _getInt(tuningValues, "c_conf_cost_diff");

        ::memcpy(tuningBuffer, &config, sizeof(REG_DVS_ME_CFG));
    }
}
