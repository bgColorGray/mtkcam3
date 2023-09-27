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
#define LOG_TAG "StereoTuningProvider_DVS_OCC"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "hw_dpe_dvs_occ_tuning.h"
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

HW_DPE_DVS_OCC_Tuning::~HW_DPE_DVS_OCC_Tuning()
{
    for(auto &t : __tuningMap) {
        if(t.second) {
            delete t.second;
        }
    }
}

bool
HW_DPE_DVS_OCC_Tuning::retrieveTuningParams(TuningQuery_T &query)
{
    DPE_Config *dst = (DPE_Config *)query.results[STEREO_TUNING_NAME[E_TUNING_HW_DPE]];

    if(NULL == dst) {
        MY_LOGE("Cannot get target");
        return false;
    }

    DVS_OCC_CFG *src = NULL;
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

    ::memcpy(&(dst->Dpe_DVSSettings.TuningBuf_OCC), src, sizeof(DVS_OCC_CFG));

    return true;
}

void
HW_DPE_DVS_OCC_Tuning::_initDefaultValues()
{
    static_assert (sizeof(REG_DVS_OCC_PQ_CFG) == sizeof(DVS_OCC_CFG), "REG_DVS_OCC_PQ_CFG Size is incorrect");
    REG_DVS_OCC_PQ_CFG config;
    config.DVS_OCC_PQ_0.Bits.c_owc_upd_uq_en     = 1;
    config.DVS_OCC_PQ_0.Bits.c_owc_uq_value      = 0;
    config.DVS_OCC_PQ_0.Bits.c_owc_en            = 1;
    config.DVS_OCC_PQ_0.Bits.c_owc_tlr_thr       = 7;

    config.DVS_OCC_PQ_1.Bits.c_ohf_upd_uq_en     = 0;
    config.DVS_OCC_PQ_1.Bits.c_ohf_uq_value      = 0;
    config.DVS_OCC_PQ_1.Bits.c_ohf_en            = 1;
    config.DVS_OCC_PQ_1.Bits.c_ohf_tlr_thr       = 64;

    config.DVS_OCC_PQ_2.Bits.c_lrc_upd_uq_en     = 1;
    config.DVS_OCC_PQ_2.Bits.c_lrc_thr_ng        = 5;
    config.DVS_OCC_PQ_2.Bits.c_lrc_thr           = 7;

    config.DVS_OCC_PQ_3.Bits.c_sbf_h_size_mode   = 0;
    config.DVS_OCC_PQ_3.Bits.c_sbf_v_size_mode   = 0;
    config.DVS_OCC_PQ_3.Bits.c_sbf_thr           = 56;

    config.DVS_OCC_PQ_4.Bits.c_ups_dv_thr        = 8;
    config.DVS_OCC_PQ_4.Bits.c_ups_bld_en        = 1;
    config.DVS_OCC_PQ_4.Bits.c_ups_bld_step      = 0;
    config.DVS_OCC_PQ_4.Bits.c_ups_weight_mode   = 1;
    config.DVS_OCC_PQ_4.Bits.c_occ_dv_min_uq_thr = 1;
    config.DVS_OCC_PQ_4.Bits.c_occ_dv_rmv_rp_en  = 1;
    config.DVS_OCC_PQ_4.Bits.c_occ_dv_msb_sft    = 0;
    config.DVS_OCC_PQ_4.Bits.c_occ_dv_lsb_sft    = 1;
    config.DVS_OCC_PQ_4.Bits.c_occ_dv_offset     = 0;
    config.DVS_OCC_PQ_4.Bits.c_occ_flag_debug    = 0;

    config.DVS_OCC_PQ_5.Bits.c_fx_baseline       = 15515287;
    config.DVS_OCC_PQ_5.Bits.c_depth_format      = 0;

    DVS_OCC_CFG *tuningBuffer = new DVS_OCC_CFG();
    ::memcpy(tuningBuffer, &config, sizeof(REG_DVS_OCC_PQ_CFG));
    __tuningMap[QUERY_KEY_PREVIEW] = tuningBuffer;
}

void
HW_DPE_DVS_OCC_Tuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "===== DVS OCC Parameters =====");
    for(auto &tuning : __tuningMap) {
        DVS_OCC_CFG *tuningBuffer = tuning.second;
        REG_DVS_OCC_PQ_CFG config;
        ::memcpy(&config, tuningBuffer, sizeof(REG_DVS_OCC_PQ_CFG));

        logger
        .FastLogD("Scenario            %s", tuning.first.c_str())
        .FastLogD("c_owc_upd_uq_en     %d", config.DVS_OCC_PQ_0.Bits.c_owc_upd_uq_en)
        .FastLogD("c_owc_uq_value      %d", config.DVS_OCC_PQ_0.Bits.c_owc_uq_value)
        .FastLogD("c_owc_en            %d", config.DVS_OCC_PQ_0.Bits.c_owc_en)
        .FastLogD("c_owc_tlr_thr       %d", config.DVS_OCC_PQ_0.Bits.c_owc_tlr_thr)
        .FastLogD("c_ohf_upd_uq_en     %d", config.DVS_OCC_PQ_1.Bits.c_ohf_upd_uq_en)
        .FastLogD("c_ohf_uq_value      %d", config.DVS_OCC_PQ_1.Bits.c_ohf_uq_value)
        .FastLogD("c_ohf_en            %d", config.DVS_OCC_PQ_1.Bits.c_ohf_en)
        .FastLogD("c_ohf_tlr_thr       %d", config.DVS_OCC_PQ_1.Bits.c_ohf_tlr_thr)
        .FastLogD("c_lrc_upd_uq_en     %d", config.DVS_OCC_PQ_2.Bits.c_lrc_upd_uq_en)
        .FastLogD("c_lrc_thr_ng        %d", config.DVS_OCC_PQ_2.Bits.c_lrc_thr_ng)
        .FastLogD("c_lrc_thr           %d", config.DVS_OCC_PQ_2.Bits.c_lrc_thr)
        .FastLogD("c_sbf_h_size_mode   %d", config.DVS_OCC_PQ_3.Bits.c_sbf_h_size_mode)
        .FastLogD("c_sbf_v_size_mode   %d", config.DVS_OCC_PQ_3.Bits.c_sbf_v_size_mode)
        .FastLogD("c_sbf_thr           %d", config.DVS_OCC_PQ_3.Bits.c_sbf_thr)
        .FastLogD("c_ups_dv_thr        %d", config.DVS_OCC_PQ_4.Bits.c_ups_dv_thr)
        .FastLogD("c_ups_bld_en        %d", config.DVS_OCC_PQ_4.Bits.c_ups_bld_en)
        .FastLogD("c_ups_bld_step      %d", config.DVS_OCC_PQ_4.Bits.c_ups_bld_step)
        .FastLogD("c_ups_weight_mode   %d", config.DVS_OCC_PQ_4.Bits.c_ups_weight_mode)
        .FastLogD("c_occ_dv_min_uq_thr %d", config.DVS_OCC_PQ_4.Bits.c_occ_dv_min_uq_thr)
        .FastLogD("c_occ_dv_rmv_rp_en  %d", config.DVS_OCC_PQ_4.Bits.c_occ_dv_rmv_rp_en)
        .FastLogD("c_occ_dv_msb_sft    %d", config.DVS_OCC_PQ_4.Bits.c_occ_dv_msb_sft)
        .FastLogD("c_occ_dv_lsb_sft    %d", config.DVS_OCC_PQ_4.Bits.c_occ_dv_lsb_sft)
        .FastLogD("c_occ_dv_offset     %d", config.DVS_OCC_PQ_4.Bits.c_occ_dv_offset)
        .FastLogD("c_occ_flag_debug    %d", config.DVS_OCC_PQ_4.Bits.c_occ_flag_debug)
        .FastLogD("c_fx_baseline       %d", config.DVS_OCC_PQ_5.Bits.c_fx_baseline)
        .FastLogD("c_depth_format      %d", config.DVS_OCC_PQ_5.Bits.c_depth_format)
        .FastLogD("%s", "------------------------------");
    }
    logger.print();
}

void
HW_DPE_DVS_OCC_Tuning::_loadValuesFromDocument(const json& dpeValues)
{
    for(auto &tuning : dpeValues) {
        DVS_OCC_CFG *tuningBuffer = NULL;
        std::string scenarioKey = tuning[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<std::string>();
        if(__tuningMap.find(scenarioKey) == __tuningMap.end()) {
            tuningBuffer = new DVS_OCC_CFG();
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
        REG_DVS_OCC_PQ_CFG config;
        config.DVS_OCC_PQ_0.Bits.c_owc_upd_uq_en     = _getInt(tuningValues, "c_owc_upd_uq_en");
        config.DVS_OCC_PQ_0.Bits.c_owc_uq_value      = _getInt(tuningValues, "c_owc_uq_value");
        config.DVS_OCC_PQ_0.Bits.c_owc_en            = _getInt(tuningValues, "c_owc_en");
        config.DVS_OCC_PQ_0.Bits.c_owc_tlr_thr       = _getInt(tuningValues, "c_owc_tlr_thr");
        config.DVS_OCC_PQ_1.Bits.c_ohf_upd_uq_en     = _getInt(tuningValues, "c_ohf_upd_uq_en");
        config.DVS_OCC_PQ_1.Bits.c_ohf_uq_value      = _getInt(tuningValues, "c_ohf_uq_value");
        config.DVS_OCC_PQ_1.Bits.c_ohf_en            = _getInt(tuningValues, "c_ohf_en");
        config.DVS_OCC_PQ_1.Bits.c_ohf_tlr_thr       = _getInt(tuningValues, "c_ohf_tlr_thr");
        config.DVS_OCC_PQ_2.Bits.c_lrc_upd_uq_en     = _getInt(tuningValues, "c_lrc_upd_uq_en");
        config.DVS_OCC_PQ_2.Bits.c_lrc_thr_ng        = _getInt(tuningValues, "c_lrc_thr_ng");
        config.DVS_OCC_PQ_2.Bits.c_lrc_thr           = _getInt(tuningValues, "c_lrc_thr");
        config.DVS_OCC_PQ_3.Bits.c_sbf_h_size_mode   = _getInt(tuningValues, "c_sbf_h_size_mode");
        config.DVS_OCC_PQ_3.Bits.c_sbf_v_size_mode   = _getInt(tuningValues, "c_sbf_v_size_mode");
        config.DVS_OCC_PQ_3.Bits.c_sbf_thr           = _getInt(tuningValues, "c_sbf_thr");
        config.DVS_OCC_PQ_4.Bits.c_ups_dv_thr        = _getInt(tuningValues, "c_ups_dv_thr");
        config.DVS_OCC_PQ_4.Bits.c_ups_bld_en        = _getInt(tuningValues, "c_ups_bld_en");
        config.DVS_OCC_PQ_4.Bits.c_ups_bld_step      = _getInt(tuningValues, "c_ups_bld_step");
        config.DVS_OCC_PQ_4.Bits.c_ups_weight_mode   = _getInt(tuningValues, "c_ups_weight_mode");
        config.DVS_OCC_PQ_4.Bits.c_occ_dv_min_uq_thr = _getInt(tuningValues, "c_occ_dv_min_uq_thr");
        config.DVS_OCC_PQ_4.Bits.c_occ_dv_rmv_rp_en  = _getInt(tuningValues, "c_occ_dv_rmv_rp_en");
        config.DVS_OCC_PQ_4.Bits.c_occ_dv_msb_sft    = _getInt(tuningValues, "c_occ_dv_msb_sft");
        config.DVS_OCC_PQ_4.Bits.c_occ_dv_lsb_sft    = _getInt(tuningValues, "c_occ_dv_lsb_sft");
        config.DVS_OCC_PQ_4.Bits.c_occ_dv_offset     = _getInt(tuningValues, "c_occ_dv_offset");
        config.DVS_OCC_PQ_4.Bits.c_occ_flag_debug    = _getInt(tuningValues, "c_occ_flag_debug");
        config.DVS_OCC_PQ_5.Bits.c_fx_baseline       = _getInt(tuningValues, "c_fx_baseline");
        config.DVS_OCC_PQ_5.Bits.c_depth_format      = _getInt(tuningValues, "c_depth_format");
        ::memcpy(tuningBuffer, &config, sizeof(REG_DVS_OCC_PQ_CFG));
    }
}
