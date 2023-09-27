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
#define LOG_TAG "GF_HAL"

#include "gf_hal_imp.h"
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VSDOF_HAL);

#include <sstream>  //For ostringstream
#include <mtkcam/utils/json/json.hpp>
#include <mtkcam/utils/json/fifo_map.hpp>
#include <fstream>

using namespace nlohmann;
// A workaround to give to use fifo_map as map, we are just ignoring the 'less' compare
template<class K, class V, class dummy_compare, class A>
using KeepOrderJsonMap = fifo_map<K, V, fifo_map_compare<K>, A>;
using KeepOrderJSON = basic_json<KeepOrderJsonMap>;

#define IS_DUMP_ENALED (NULL != __dumpHint)

void
GF_HAL_IMP::__dumpInitInfo()
{
    if(!IS_DUMP_ENALED) {
        return;
    }

    if(__initDumpSize < 1) {
        KeepOrderJSON jsonObj;
        jsonObj["gfMode"]              = __initInfo.gfMode;
        jsonObj["inputWidth"]          = __initInfo.inputWidth;
        jsonObj["inputHeight"]         = __initInfo.inputHeight;
        jsonObj["outputWidth"]         = __initInfo.outputWidth;
        jsonObj["outputHeight"]        = __initInfo.outputHeight;
        jsonObj["workingBuffSize"]     = __initInfo.workingBuffSize;
        jsonObj["cam"]["baseline"]     = __initInfo.cam.baseline;
        jsonObj["cam"]["focalLength"]  = __initInfo.cam.focalLength;
        jsonObj["cam"]["fNumber"]      = __initInfo.cam.fNumber;
        jsonObj["cam"]["sensorWidth"]  = __initInfo.cam.sensorWidth;
        jsonObj["cam"]["pixelArrayW"]  = __initInfo.cam.pixelArrayW;
        jsonObj["cam"]["sensorSizeW"]  = __initInfo.cam.sensorSizeW;
        jsonObj["cam"]["sensorScaleW"] = __initInfo.cam.sensorScaleW;
        jsonObj["cam"]["rrzUsageW"]    = __initInfo.cam.rrzUsageW;
        jsonObj["cam"]["renderWidth"]  = __initInfo.cam.renderWidth;
        jsonObj["nvram"]               = __initInfo.nvram;
        __dumpInitData = jsonObj.dump(4);   //set indent of space
        __initDumpSize = __dumpInitData.length();
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "GF_INIT_INFO.json");
    FILE *fp = fopen(dumpPath, "w");
    if(NULL == fp) {
        MY_LOGE("Cannot dump GF init info to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(__dumpInitData.c_str(), 1, __initDumpSize, fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp), "Flush failed");
    MY_LOGE_IF(fclose(fp), "Close failed");
}

void
GF_HAL_IMP::__dumpProcInfo()
{
    if(!IS_DUMP_ENALED) {
        return;
    }

    KeepOrderJSON jsonObj;

    // Proc info
    jsonObj["frame"]["sn"]            = __procInfo.frame.sn;
    jsonObj["frame"]["id"]            = __procInfo.frame.id;
    jsonObj["dof"]                    = __procInfo.dof;
    jsonObj["fb"]                     = __procInfo.fb;
    jsonObj["cOffset"]                = __procInfo.cOffset;
    jsonObj["dacInfo"]["min"]         = __procInfo.dacInfo.min;
    jsonObj["dacInfo"]["max"]         = __procInfo.dacInfo.max;
    jsonObj["dacInfo"]["cur"]         = __procInfo.dacInfo.cur;
    jsonObj["afInfo"]["afType"]       = __procInfo.afInfo.afType;
    jsonObj["afInfo"]["touchTrigger"] = __procInfo.afInfo.touchTrigger;
    jsonObj["afInfo"]["box0"]["x"]    = __procInfo.afInfo.box0.x;
    jsonObj["afInfo"]["box0"]["y"]    = __procInfo.afInfo.box0.y;
    jsonObj["afInfo"]["box1"]["x"]    = __procInfo.afInfo.box1.x;
    jsonObj["afInfo"]["box1"]["y"]    = __procInfo.afInfo.box1.y;
    jsonObj["afInfo"]["touch"]["x"]   = __procInfo.afInfo.touch.x;
    jsonObj["afInfo"]["touch"]["y"]   = __procInfo.afInfo.touch.y;
    jsonObj["zoomRatioP1"]            = __procInfo.zoomRatioP1;
    jsonObj["zoomRatioP2"]            = __procInfo.zoomRatioP2;
    jsonObj["faceNum"]                = __procInfo.faceNum;

    // Proc info
    jsonObj["face"] = json::array();
    for(size_t i = 0; i < __procInfo.faceNum; ++i)
    {
        KeepOrderJSON faceJsonObj;
        faceJsonObj["conf"]           = __procInfo.face[i].conf;
        faceJsonObj["rip"]            = __procInfo.face[i].rip;
        faceJsonObj["rop"]            = __procInfo.face[i].rop;
        faceJsonObj["visiblePercent"] = __procInfo.face[i].visiblePercent;
        faceJsonObj["box0"]["x"]      = __procInfo.face[i].box0.x;
        faceJsonObj["box0"]["y"]      = __procInfo.face[i].box0.y;
        faceJsonObj["box1"]["x"]      = __procInfo.face[i].box1.x;
        faceJsonObj["box1"]["y"]      = __procInfo.face[i].box1.y;
        faceJsonObj["eyeL0"]["x"]     = __procInfo.face[i].eyeL0.x;
        faceJsonObj["eyeL0"]["y"]     = __procInfo.face[i].eyeL0.y;
        faceJsonObj["eyeL1"]["x"]     = __procInfo.face[i].eyeL1.x;
        faceJsonObj["eyeL1"]["y"]     = __procInfo.face[i].eyeL1.y;
        faceJsonObj["eyeR0"]["x"]     = __procInfo.face[i].eyeR0.x;
        faceJsonObj["eyeR0"]["y"]     = __procInfo.face[i].eyeR0.y;
        faceJsonObj["eyeR1"]["x"]     = __procInfo.face[i].eyeR1.x;
        faceJsonObj["eyeR1"]["y"]     = __procInfo.face[i].eyeR1.y;
        faceJsonObj["nose"]["x"]      = __procInfo.face[i].nose.x;
        faceJsonObj["nose"]["y"]      = __procInfo.face[i].nose.y;
        faceJsonObj["mouth0"]["x"]    = __procInfo.face[i].mouth0.x;
        faceJsonObj["mouth0"]["y"]    = __procInfo.face[i].mouth0.y;
        faceJsonObj["mouth1"]["x"]    = __procInfo.face[i].mouth1.x;
        faceJsonObj["mouth1"]["y"]    = __procInfo.face[i].mouth1.y;
        jsonObj["face"].push_back(faceJsonObj);
    }

    std::string s = jsonObj.dump(4);   //set indent of space
    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "GF_PROC_INFO.json");
    FILE *fp = fopen(dumpPath, "w");
    if(NULL == fp) {
        MY_LOGE("Cannot dump GF proc info to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(s.c_str(), 1, s.length(), fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp), "Flush failed");
    MY_LOGE_IF(fclose(fp), "Close failed");
}

void
GF_HAL_IMP::__dumpResultInfo()
{
    if(!IS_DUMP_ENALED) {
        return;
    }

    KeepOrderJSON jsonObj;

    // Init info
    jsonObj["GFInitInfo"]["gfMode"]              = __initInfo.gfMode;
    jsonObj["GFInitInfo"]["inputWidth"]          = __initInfo.inputWidth;
    jsonObj["GFInitInfo"]["inputHeight"]         = __initInfo.inputHeight;
    jsonObj["GFInitInfo"]["outputWidth"]         = __initInfo.outputWidth;
    jsonObj["GFInitInfo"]["outputHeight"]        = __initInfo.outputHeight;
    jsonObj["GFInitInfo"]["workingBuffSize"]     = __initInfo.workingBuffSize;
    jsonObj["GFInitInfo"]["cam"]["baseline"]     = __initInfo.cam.baseline;
    jsonObj["GFInitInfo"]["cam"]["focalLength"]  = __initInfo.cam.focalLength;
    jsonObj["GFInitInfo"]["cam"]["fNumber"]      = __initInfo.cam.fNumber;
    jsonObj["GFInitInfo"]["cam"]["sensorWidth"]  = __initInfo.cam.sensorWidth;
    jsonObj["GFInitInfo"]["cam"]["pixelArrayW"]  = __initInfo.cam.pixelArrayW;
    jsonObj["GFInitInfo"]["cam"]["sensorSizeW"]  = __initInfo.cam.sensorSizeW;
    jsonObj["GFInitInfo"]["cam"]["sensorScaleW"] = __initInfo.cam.sensorScaleW;
    jsonObj["GFInitInfo"]["cam"]["rrzUsageW"]    = __initInfo.cam.rrzUsageW;
    jsonObj["GFInitInfo"]["cam"]["renderWidth"]  = __initInfo.cam.renderWidth;
    jsonObj["GFInitInfo"]["nvram"]               = __initInfo.nvram;

    // Proc info
    jsonObj["GFProcInfo"]["frame"]["sn"]               = __procInfo.frame.sn;
    jsonObj["GFProcInfo"]["frame"]["id"]               = __procInfo.frame.id;
    jsonObj["GFProcInfo"]["dof"]                       = __procInfo.dof;
    jsonObj["GFProcInfo"]["fb"]                        = __procInfo.fb;
    jsonObj["GFProcInfo"]["cOffset"]                   = __procInfo.cOffset;
    jsonObj["GFProcInfo"]["dacInfo"]["min"]            = __procInfo.dacInfo.min;
    jsonObj["GFProcInfo"]["dacInfo"]["max"]            = __procInfo.dacInfo.max;
    jsonObj["GFProcInfo"]["dacInfo"]["cur"]            = __procInfo.dacInfo.cur;
    jsonObj["GFProcInfo"]["afInfo"]["afType"]          = __procInfo.afInfo.afType;
    jsonObj["GFProcInfo"]["afInfo"]["touchTrigger"]    = __procInfo.afInfo.touchTrigger;
    jsonObj["GFProcInfo"]["afInfo"]["box0"]["x"]       = __procInfo.afInfo.box0.x;
    jsonObj["GFProcInfo"]["afInfo"]["box0"]["y"]       = __procInfo.afInfo.box0.y;
    jsonObj["GFProcInfo"]["afInfo"]["box1"]["x"]       = __procInfo.afInfo.box1.x;
    jsonObj["GFProcInfo"]["afInfo"]["box1"]["y"]       = __procInfo.afInfo.box1.y;
    jsonObj["GFProcInfo"]["afInfo"]["touch"]["x"]      = __procInfo.afInfo.touch.x;
    jsonObj["GFProcInfo"]["afInfo"]["touch"]["y"]      = __procInfo.afInfo.touch.y;
    jsonObj["GFProcInfo"]["afInfo"]["pdCoverage"]["x"] = __procInfo.afInfo.pdCoverage.x;
    jsonObj["GFProcInfo"]["afInfo"]["pdCoverage"]["y"] = __procInfo.afInfo.pdCoverage.y;
    jsonObj["GFProcInfo"]["afInfo"]["pdGridNum"]["x"]  = __procInfo.afInfo.pdGridNum.x;
    jsonObj["GFProcInfo"]["afInfo"]["pdGridNum"]["y"]  = __procInfo.afInfo.pdGridNum.y;
    const size_t PD_MAP_SIZE = __procInfo.afInfo.pdGridNum.x * __procInfo.afInfo.pdGridNum.y;
    std::vector<MUINT32> pdMap;
    if(PD_MAP_SIZE > 0) {
        pdMap = std::move(std::vector<MUINT32>(__procInfo.afInfo.pdLable,
                                               __procInfo.afInfo.pdLable + PD_MAP_SIZE));
    }
    jsonObj["GFProcInfo"]["afInfo"]["pdLable"]      = pdMap;
    jsonObj["GFProcInfo"]["zoomRatioP1"]            = __procInfo.zoomRatioP1;
    jsonObj["GFProcInfo"]["zoomRatioP2"]            = __procInfo.zoomRatioP2;
    jsonObj["GFProcInfo"]["faceNum"]                = __procInfo.faceNum;

    // Proc info
    jsonObj["GFProcInfo"]["face"] = json::array();
    for(size_t i = 0; i < __procInfo.faceNum; ++i)
    {
        KeepOrderJSON faceJsonObj;
        faceJsonObj["conf"]           = __procInfo.face[i].conf;
        faceJsonObj["rip"]            = __procInfo.face[i].rip;
        faceJsonObj["rop"]            = __procInfo.face[i].rop;
        faceJsonObj["visiblePercent"] = __procInfo.face[i].visiblePercent;
        faceJsonObj["box0"]["x"]      = __procInfo.face[i].box0.x;
        faceJsonObj["box0"]["y"]      = __procInfo.face[i].box0.y;
        faceJsonObj["box1"]["x"]      = __procInfo.face[i].box1.x;
        faceJsonObj["box1"]["y"]      = __procInfo.face[i].box1.y;
        faceJsonObj["eyeL0"]["x"]     = __procInfo.face[i].eyeL0.x;
        faceJsonObj["eyeL0"]["y"]     = __procInfo.face[i].eyeL0.y;
        faceJsonObj["eyeL1"]["x"]     = __procInfo.face[i].eyeL1.x;
        faceJsonObj["eyeL1"]["y"]     = __procInfo.face[i].eyeL1.y;
        faceJsonObj["eyeR0"]["x"]     = __procInfo.face[i].eyeR0.x;
        faceJsonObj["eyeR0"]["y"]     = __procInfo.face[i].eyeR0.y;
        faceJsonObj["eyeR1"]["x"]     = __procInfo.face[i].eyeR1.x;
        faceJsonObj["eyeR1"]["y"]     = __procInfo.face[i].eyeR1.y;
        faceJsonObj["nose"]["x"]      = __procInfo.face[i].nose.x;
        faceJsonObj["nose"]["y"]      = __procInfo.face[i].nose.y;
        faceJsonObj["mouth0"]["x"]    = __procInfo.face[i].mouth0.x;
        faceJsonObj["mouth0"]["y"]    = __procInfo.face[i].mouth0.y;
        faceJsonObj["mouth1"]["x"]    = __procInfo.face[i].mouth1.x;
        faceJsonObj["mouth1"]["y"]    = __procInfo.face[i].mouth1.y;
        jsonObj["GFProcInfo"]["face"].push_back(faceJsonObj);
    }

    // Result info
    jsonObj["GFResultInfo"]["focusStatus"] = __resultInfo.focusStatus;
    jsonObj["GFResultInfo"]["bgStatus"]    = __resultInfo.bgStatus;
    // jsonObj["GFResultInfo"]["lowConf"]     = __resultInfo.lowConf;
    // jsonObj["GFResultInfo"]["tracking"]    = __resultInfo.tracking;
    jsonObj["GFResultInfo"]["transition"]  = __resultInfo.transition;
    // jsonObj["GFResultInfo"]["convergence"] = __resultInfo.convergence;
    jsonObj["GFResultInfo"]["converge"]["value"]             = __resultInfo.converge.value;
    jsonObj["GFResultInfo"]["converge"]["value_tar"]         = __resultInfo.converge.value_tar;
    jsonObj["GFResultInfo"]["converge"]["value_n3d"]         = __resultInfo.converge.value_n3d;
    jsonObj["GFResultInfo"]["converge"]["value_cur"]         = __resultInfo.converge.value_cur;
    jsonObj["GFResultInfo"]["converge"]["sr_lower"]          = __resultInfo.converge.sr_lower;
    jsonObj["GFResultInfo"]["converge"]["sr_upper"]          = __resultInfo.converge.sr_upper;
    jsonObj["GFResultInfo"]["converge"]["hist_total_amount"] = __resultInfo.converge.hist_total_amount;
    jsonObj["GFResultInfo"]["converge"]["hist_total_var"]    = __resultInfo.converge.hist_total_var;
    jsonObj["GFResultInfo"]["converge"]["hist_total_shift"]  = __resultInfo.converge.hist_total_shift;
    jsonObj["GFResultInfo"]["converge"]["hist_total"]        = __resultInfo.converge.hist_total;
    jsonObj["GFResultInfo"]["converge"]["hist_cur"]          = __resultInfo.converge.hist_cur;
    jsonObj["GFResultInfo"]["converge"]["hist_ori"]          = __resultInfo.converge.hist_ori;
    jsonObj["GFResultInfo"]["converge"]["found"]             = __resultInfo.converge.found;
    jsonObj["GFResultInfo"]["converge"]["amount_score"]      = __resultInfo.converge.amount_score;
    jsonObj["GFResultInfo"]["converge"]["valid_score"]       = __resultInfo.converge.valid_score;
    jsonObj["GFResultInfo"]["converge"]["var_score"]         = __resultInfo.converge.var_score;
    jsonObj["GFResultInfo"]["converge"]["amount"]            = __resultInfo.converge.amount;
    jsonObj["GFResultInfo"]["converge"]["valid"]             = __resultInfo.converge.valid;
    jsonObj["GFResultInfo"]["converge"]["var"]               = __resultInfo.converge.var;
    jsonObj["GFResultInfo"]["converge"]["lower"]             = __resultInfo.converge.lower;
    jsonObj["GFResultInfo"]["converge"]["upper"]             = __resultInfo.converge.upper;
    jsonObj["GFResultInfo"]["converge"]["cluster_conf"]      = __resultInfo.converge.cluster_conf;
    jsonObj["GFResultInfo"]["converge"]["amount_conf_total"] = __resultInfo.converge.amount_conf_total;
    jsonObj["GFResultInfo"]["converge"]["learning_rate"]     = __resultInfo.converge.learning_rate;
    jsonObj["GFResultInfo"]["converge"]["found_counter"]     = __resultInfo.converge.found_counter;

    jsonObj["GFResultInfo"]["bokehAf"]["enable"]    = __resultInfo.bokehAf.enable;
    jsonObj["GFResultInfo"]["bokehAf"]["dac"]       = __resultInfo.bokehAf.dac;
    jsonObj["GFResultInfo"]["bokehAf"]["conf"]      = __resultInfo.bokehAf.conf;
    jsonObj["GFResultInfo"]["bokehAf"]["box0"]["x"] = __resultInfo.bokehAf.box0.x;
    jsonObj["GFResultInfo"]["bokehAf"]["box0"]["y"] = __resultInfo.bokehAf.box0.y;
    jsonObj["GFResultInfo"]["bokehAf"]["box1"]["x"] = __resultInfo.bokehAf.box1.x;
    jsonObj["GFResultInfo"]["bokehAf"]["box1"]["y"] = __resultInfo.bokehAf.box1.y;
    jsonObj["GFResultInfo"]["bokehAf"]["focus_ori"] = __resultInfo.bokehAf.focus_ori;
    jsonObj["GFResultInfo"]["bokehAf"]["focus"]     = __resultInfo.bokehAf.focus;
    jsonObj["GFResultInfo"]["bokehAf"]["th"]        = __resultInfo.bokehAf.th;
    jsonObj["GFResultInfo"]["bokehAf"]["var"]       = __resultInfo.bokehAf.var;
    jsonObj["GFResultInfo"]["bokehAf"]["hist"]      = __resultInfo.bokehAf.hist;

    jsonObj["GFResultInfo"]["dof"]         = __resultInfo.dof;
    jsonObj["GFResultInfo"]["focus"]       = __resultInfo.focus;
    jsonObj["GFResultInfo"]["clr"]         = __resultInfo.clr;
    jsonObj["GFResultInfo"]["gain"]        = __resultInfo.gain;
    jsonObj["GFResultInfo"]["nvram"]       = __resultInfo.nvram;

    std::string s = jsonObj.dump(4);   //set indent of space
    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "GF_RESULT_INFO.json");
    FILE *fp = fopen(dumpPath, "w");
    if(NULL == fp) {
        MY_LOGE("Cannot dump GF result info to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(s.c_str(), 1, s.length(), fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp), "Flush failed");
    MY_LOGE_IF(fclose(fp), "Close failed");
}

void
GF_HAL_IMP::__dumpWorkingBuffer()
{
    if(!IS_DUMP_ENALED ||
       checkStereoProperty("vendor.STEREO.dump.gf_wb") < 1)
    {
        return;
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "GF_WORKING_BUFFER");
    FILE *fp = fopen(dumpPath, "wb");
    if(NULL == fp) {
        MY_LOGE("Cannot dump GF working buffer to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(__initInfo.workingBuffAddr, 1, __initInfo.workingBuffSize, fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp), "Flush failed");
    MY_LOGE_IF(fclose(fp), "Close failed");
}
