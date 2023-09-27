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
#define LOG_TAG "StereoTuningProvider_SWBLENDING"

#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_MULTICAM_PROVIDER
#include "sw_blending_tuning.h"

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

SW_BLENDINGTuning::SW_BLENDINGTuning(json &tuningJson)
{
    __blendingMap[eSTEREO_SCENARIO_PREVIEW] = SW_BLENDING_TUNING_T();
    __blendingMap[eSTEREO_SCENARIO_RECORD]  = SW_BLENDING_TUNING_T();
    __blendingMap[eSTEREO_SCENARIO_CAPTURE] = SW_BLENDING_TUNING_T();

    _init(tuningJson);
}

SW_BLENDINGTuning::~SW_BLENDINGTuning()
{
}

bool
SW_BLENDINGTuning::retrieveTuningParams(TuningQuery_T &query)
{
    int *coreNumber = NULL;
    if(query.results.find(QUERY_KEY_CORE_NUMBER) != query.results.end()) {
        coreNumber = (int *)query.results[QUERY_KEY_CORE_NUMBER];
    }

    ALPHATABLE_T *pAlphaTable = NULL;
    if(query.results.find(QUERY_KEY_ALPHATABLE) != query.results.end()) {
        pAlphaTable = (ALPHATABLE_T *)query.results[QUERY_KEY_ALPHATABLE];
    }

    TUNING_PAIR_LIST_T *params = NULL;
    if(query.results.find(QUERY_KEY_TUNING_PARAMS) != query.results.end()) {
        params = (TUNING_PAIR_LIST_T *)query.results[QUERY_KEY_TUNING_PARAMS];
    }

    bool result = true;
    int scenario = query.intParams[QUERY_KEY_SCENARIO];
    auto iter = __blendingMap.find(scenario);
    if(iter == __blendingMap.end() &&
       scenario == eSTEREO_SCENARIO_RECORD)
    {
        iter = __blendingMap.find(eSTEREO_SCENARIO_PREVIEW);
        MY_LOGD_IF(iter != __blendingMap.end(), "Record is not configured in tuning, use preview instead");
    }

    if(iter != __blendingMap.end()) {
        SW_BLENDING_TUNING_T &tuning = iter->second;
        if(coreNumber) {
            *coreNumber = tuning.coreNumber;
        }

        if(pAlphaTable) {
            *pAlphaTable = tuning.alphaTable;
        }

        if(params) {
            *params = tuning.params;
        }
    } else {
        MY_LOGE("Unknown scenario: %d", scenario);
        result = false;
    }

    return result;
}

void
SW_BLENDINGTuning::_initDefaultValues()
{
    for(int i = 1; i < TOTAL_STEREO_SCENARIO; i++) {
        SW_BLENDING_TUNING_T &blendingTuning = __blendingMap[i];
        blendingTuning.coreNumber     = 1;
        blendingTuning.alphaTable     = __DEFAULT_ALPHA_TABLE;
        blendingTuning.params =
        {
        };
    }
}

void
SW_BLENDINGTuning::log(FastLogger &logger, bool inJSON)
{
    if(inJSON) {
        return StereoTuningBase::log(logger, inJSON);
    }

    logger.FastLogD("%s", "======== Blending Parameters ========");
    for (BLENDING_MAP_T::iterator it = __blendingMap.begin(); it != __blendingMap.end(); ++it) {
        MUINT32 scenario = it->first;
        SW_BLENDING_TUNING_T &blendingTuning = it->second;

        logger
        .FastLogD("Scenario: %s", SCENARIO_NAMES[scenario])
        .FastLogD("CoreNumber: %d", blendingTuning.coreNumber);

        std::ostringstream oss;
        if(blendingTuning.alphaTable.size() > 0)
        {
            for(size_t i = 0; i < blendingTuning.alphaTable.size(); ++i) {
                oss << blendingTuning.alphaTable[i];
                if((i+1) % 8 != 0) {
                    oss << " ";
                } else {
                    oss << "\n            ";
                }
            }
            logger.FastLogD("Alpha Table(%zu): %s", blendingTuning.alphaTable.size(), oss.str().c_str());
        }

        for(auto &param : blendingTuning.params) {
            logger.FastLogD("\"%s\": %d", param.first.c_str(), param.second);
        }

        logger.FastLogD("%s", "-------------------------------");
    }

    logger.print();
}

void
SW_BLENDINGTuning::_loadValuesFromDocument(const json& blendingValues)
{
    const char *TUNING_PREFIX = "blending.";
    const size_t TUNING_PREFIX_SIZE = strlen(TUNING_PREFIX);

    for(auto &blendingValue : blendingValues) {
        std::string SCENARIO_NAME = blendingValue[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO].get<string>();
        int scenario = 1;
        for(int s = 1; s < TOTAL_STEREO_SCENARIO; s++) {
            if(blendingValue[VALUE_KEY_PARAMETERS][QUERY_KEY_SCENARIO] == SCENARIO_NAMES[s]) {
                scenario = s;
                break;
            }
        }

        if(__blendingMap.find(scenario) == __blendingMap.end()) {
            MY_LOGE("Unkown scenario %s", SCENARIO_NAME.c_str());
            continue;
        }

        SW_BLENDING_TUNING_T &tuning = __blendingMap[scenario];
        const json &tuningValues = blendingValue[VALUE_KEY_VALUES];
        if(LogicalCamJSONUtil::HasMember(tuningValues, QUERY_KEY_CORE_NUMBER))
        {
            tuning.coreNumber = tuningValues[QUERY_KEY_CORE_NUMBER].get<int>();
        }

        if(tuning.coreNumber < 1) {
            tuning.coreNumber = 1;
        }

        //Extract tuning params
        bool hasAlphaTableArray     = false;
        tuning.alphaTable.clear();

        int v = 0;
        if(_isArray(tuningValues, QUERY_KEY_ALPHATABLE)) {
            hasAlphaTableArray = true;
            tuning.alphaTable = tuningValues[QUERY_KEY_ALPHATABLE].get<ALPHATABLE_T>();
        } else {
            int alphaTableIndex  = 0;
            string alphaTableKey = QUERY_KEY_ALPHATABLE+std::to_string(alphaTableIndex);
            while(LogicalCamJSONUtil::HasMember(tuningValues, alphaTableKey)) {
                v = tuningValues[alphaTableKey].get<MUINT8>();
                if(v > 0) {
                    tuning.alphaTable.push_back(v);
                } else {
                    MY_LOGW("Ignore invalid alpha range(%d) for index %d", v, alphaTableIndex);
                }

                alphaTableIndex++;
                alphaTableKey = QUERY_KEY_ALPHATABLE+std::to_string(alphaTableIndex);
            }
        }

        tuning.params.clear();
        for(json::const_iterator it = tuningValues.begin(); it != tuningValues.end(); ++it) {
            //Extract "blending.xxxx" parameters
            if(it.key().length() > TUNING_PREFIX_SIZE &&
               !strncmp(it.key().c_str(), TUNING_PREFIX, TUNING_PREFIX_SIZE))
            {
                tuning.params.push_back({it.key(), it.value().get<int>()});
            }
        }
    }
}
