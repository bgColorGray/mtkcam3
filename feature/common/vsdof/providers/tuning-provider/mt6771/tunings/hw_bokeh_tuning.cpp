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
#define LOG_TAG "StereoTuningProvider_HWBokeh"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "hw_bokeh_tuning.h"

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

const char *QUERY_KEY_BOKEH_STRENTH_NAMES[] =
{
    "Weak",
    "Normal",
    "Strong"
};

HW_BOKEHTuning::HW_BOKEHTuning(json &tuningJson)
{
    // __bokehMap[E_BOKEH_STRENGTH_WEAK]   = &weakSetting;
    __bokehMap[E_BOKEH_STRENGTH_NORMAL] = &normalSetting;
    __bokehMap[E_BOKEH_STRENGTH_STRONG] = &strongSetting;

    _init(tuningJson);
}

HW_BOKEHTuning::~HW_BOKEHTuning()
{
}

bool
HW_BOKEHTuning::retrieveTuningParams(TuningQuery_T &query)
{
    dip_x_reg_t *bokehInfo = (dip_x_reg_t *)query.results[QUERY_KEY_HWBOKEH_INFO];

    if(NULL == bokehInfo) {
        MY_LOGE("Cannot get %s", QUERY_KEY_HWBOKEH_INFO);
        return false;
    }

    bool result = true;
    int strength = query.intParams[QUERY_KEY_HWBOKEH_STRENTH];
    if(__bokehMap.find(strength) != __bokehMap.end()) {
        dip_x_reg_t *queryResult = __bokehMap[strength];

        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_MODE      = 0;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE   = queryResult->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE = queryResult->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT   = queryResult->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN     = queryResult->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT    = queryResult->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN   = queryResult->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY = queryResult->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M     = queryResult->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF      = queryResult->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF      = queryResult->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF;
    } else {
        MY_LOGE("Unknown strength: %d", strength);
        result = false;
    }

    return result;
}

void
HW_BOKEHTuning::_initDefaultValues()
{
    {
        dip_x_reg_t *bokehInfo = __bokehMap[E_BOKEH_STRENGTH_STRONG];
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE   = 0;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE = 0;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT   = 4;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN     = 1;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT    = 12;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN   = 8;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY = 240;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M     = 1;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF      = 0;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF      = 0;
    }

    {
        dip_x_reg_t *bokehInfo = __bokehMap[E_BOKEH_STRENGTH_NORMAL];
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE   = 0;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE = 0;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT   = 4;
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN     = 1;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT    = 63;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN   = 63;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY = 255;
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M     = 1;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF      = 0;
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF      = 0;
    }
}

void
HW_BOKEHTuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "============ HW Bokeh Parameters ============");
    for (HW_BOKEH_MAP_T::iterator it = __bokehMap.begin(); it != __bokehMap.end(); ++it) {
        size_t strength = it->first;
        dip_x_reg_t *bokehInfo = it->second;

        logger
        .FastLogD("Strength: %s", QUERY_KEY_BOKEH_STRENTH_NAMES[(size_t)strength])
        .FastLogD("DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE:   %d", bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE)
        .FastLogD("DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE: %d", bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE)
        .FastLogD("DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT:   %d", bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT)
        .FastLogD("DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN:     %d", bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN)
        .FastLogD("DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT:    %d", bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT)
        .FastLogD("DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN:   %d", bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN)
        .FastLogD("DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY: %d", bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY)
        .FastLogD("DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M:     %d", bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M)
        .FastLogD("DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF:      %d", bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF)
        .FastLogD("DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF:      %d", bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF)
        .FastLogD("%s", "---------------------------------------");
    }

    logger.print();
}

void
HW_BOKEHTuning::_loadValuesFromDocument(const json& bokehValues)
{
    for(auto &tuning : bokehValues)
    {
        int strength = 0;
        for(int s = 0; s < TOTAL_BOKEH_STRENGTH; s++) {
            if(tuning[VALUE_KEY_PARAMETERS][QUERY_KEY_HWBOKEH_STRENTH] == QUERY_KEY_BOKEH_STRENTH_NAMES[s])
            {
                strength = s;
                break;
            }
        }

        if(__bokehMap.find(strength) == __bokehMap.end()) {
            MY_LOGE("Unkown strength %d", strength);
            continue;
        }

        dip_x_reg_t *bokehInfo = __bokehMap[strength];
        const json &tuningValues = tuning[VALUE_KEY_VALUES];
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE   = _getInt(tuningValues, "DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_AP_MODE");
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE = _getInt(tuningValues, "DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_MODE");
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT   = _getInt(tuningValues, "DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_FGBG_WT");
        bokehInfo->DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN     = _getInt(tuningValues, "DIP_X_NBC2_BOK_CON.Bits.NBC2_BOK_PF_EN");
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT    = _getInt(tuningValues, "DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_STR_WT");
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN   = _getInt(tuningValues, "DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_WT_GAIN");
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY = _getInt(tuningValues, "DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_INTENSITY");
        bokehInfo->DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M     = _getInt(tuningValues, "DIP_X_NBC2_BOK_TUN.Bits.NBC2_BOK_DOF_M");
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF      = _getInt(tuningValues, "DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_XOFF");
        bokehInfo->DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF      = _getInt(tuningValues, "DIP_X_NBC2_BOK_OFF.Bits.NBC2_BOK_YOFF");
    }
}
