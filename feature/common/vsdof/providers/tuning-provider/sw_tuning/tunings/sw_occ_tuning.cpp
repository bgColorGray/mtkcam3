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
#define LOG_TAG "StereoTuningProvider_SWOCC"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "sw_occ_tuning.h"

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

SW_OCCTuning::SW_OCCTuning(json &tuningJson)
{
    __occMap[eSTEREO_SCENARIO_PREVIEW] = SW_OCC_TUNING_T();
    __occMap[eSTEREO_SCENARIO_RECORD]  = SW_OCC_TUNING_T();
    __occMap[eSTEREO_SCENARIO_CAPTURE] = SW_OCC_TUNING_T();

    _init(tuningJson);
}

SW_OCCTuning::~SW_OCCTuning()
{
}

bool
SW_OCCTuning::retrieveTuningParams(TuningQuery_T &query)
{
    int *coreNumber = (int *)query.results[QUERY_KEY_CORE_NUMBER];
    TUNING_PAIR_LIST_T *params = (TUNING_PAIR_LIST_T *)query.results[QUERY_KEY_TUNING_PARAMS];

    if(NULL == params) {
        MY_LOGE("Cannot get %s", QUERY_KEY_TUNING_PARAMS);
        return false;
    }

    bool result = true;
    int scenario = query.intParams[QUERY_KEY_SCENARIO];
    if(__occMap.find(scenario) != __occMap.end()) {
        *coreNumber = __occMap[scenario].coreNumber;
        *params = __occMap[scenario].params;
    } else {
        MY_LOGE("Unknown scenario: %d", scenario);
        result = false;
    }

    return result;
}

void
SW_OCCTuning::_initDefaultValues()
{
    for(int i = 1; i < TOTAL_STEREO_SCENARIO; i++) {
        SW_OCC_TUNING_T &occTuning = __occMap[i];
        occTuning.coreNumber = 1;
        occTuning.params =
        {
            {"occ.debugLevel",  0},
            {"occ.logLevel",    0},
            {"occ.calibration_en", 0},
            {"occ.offset_en", 0},
        };

        if(i == eSTEREO_SCENARIO_CAPTURE) {
            occTuning.params.push_back({"occ.holeFilling", 1});
        }
    }
}

void
SW_OCCTuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "======== OCC Parameters ========");
    for (OCC_MAP_T::iterator it = __occMap.begin(); it != __occMap.end(); ++it) {
        MUINT32 scenario = it->first;
        SW_OCC_TUNING_T &occTuning = it->second;

        logger
        .FastLogD("Scenario: %s", SCENARIO_NAMES[scenario])
        .FastLogD("CoreNumber: %d", occTuning.coreNumber);

        for(auto &param : occTuning.params) {
            logger.FastLogD("\"%s\": %d", param.first.c_str(), param.second);
        }

        logger.FastLogD("%s", "-------------------------------");
    }

    logger.print();
}

void
SW_OCCTuning::_loadValuesFromDocument(const json& occValues)
{
    const char *TUNING_PREFIX = "occ.";
    const size_t TUNING_PREFIX_SIZE = strlen(TUNING_PREFIX);

    for(auto &occValue : occValues) {
        int scenario = 1;
        for(MUINT32 s = 1; s < TOTAL_STEREO_SCENARIO; s++) {
            if(occValue[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO] == SCENARIO_NAMES[s]) {
                scenario = s;
                break;
            }
        }

        if(__occMap.find(scenario) == __occMap.end()) {
            std::string SCENARIO_NAME = occValue[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<string>();
            MY_LOGE("Unkown scenario %s", SCENARIO_NAME.c_str());
            continue;
        }

        SW_OCC_TUNING_T &tuning = __occMap[scenario];
        const json &tuningValues = occValue[VALUE_KEY_VALUES];
        tuning.coreNumber = tuningValues[QUERY_KEY_OCC_CORE_NUMBER].get<int>();
        if(tuning.coreNumber < 1) {
            tuning.coreNumber = 1;
        }

        //Extract tuning params
        tuning.params.clear();
        for(json::const_iterator it = tuningValues.begin(); it != tuningValues.end(); ++it) {
            if(it.key().length() > TUNING_PREFIX_SIZE &&
               !strncmp(it.key().c_str(), TUNING_PREFIX, TUNING_PREFIX_SIZE))
            {
                tuning.params.push_back({it.key(), it.value().get<int>()});
            }
        }
    }
}
