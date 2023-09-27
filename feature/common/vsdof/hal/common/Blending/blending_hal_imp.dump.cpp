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
#define LOG_TAG "BLENDING_HAL"

#include "blending_hal_imp.h"
#include <mtkcam/utils/std/ULog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_VSDOF_HAL);

#define IS_DUMP_ENALED (NULL != __dumpHint)

void
BLENDING_HAL_IMP::__dumpInitInfo()
{
    if(!IS_DUMP_ENALED) {
        return;
    }

    if(__initDumpSize < 1) {
        if(snprintf(__dumpInitData,
                    __DUMP_BUFFER_SIZE,
                    "{\n"
                    "    \"Width\": %d,\n"
                    "    \"Height\": %d,\n"
                    "    \"BlurWidth\": %d,\n"
                    "    \"BlurHeight\": %d,\n"
                    "    \"workingBuffSize\": %d\n"
                    "}",
                    __initInfo.Width,
                    __initInfo.Height,
                    __initInfo.BlurWidth,
                    __initInfo.BlurHeight,
                    __initInfo.workingBuffSize) > 0)
        {
            __initDumpSize = strlen(__dumpInitData);
        }
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "BLENDING_INIT_INFO.json");
    FILE *fp = fopen(dumpPath, "w");
    if(NULL == fp) {
        MY_LOGE("Cannot dump BLENDING init info to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(__dumpInitData, 1, __initDumpSize, fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp) != 0, "Flush failed");
    MY_LOGE_IF(fclose(fp) != 0, "Close failed");
}

void
BLENDING_HAL_IMP::__dumpProcInfo()
{
    if(!IS_DUMP_ENALED) {
        return;
    }

    // snprintf(__dumpProcData,
    //          __DUMP_BUFFER_SIZE,
    //         "{\n"
    //         "}"
    //         );

    // char dumpPath[PATH_MAX];
    // genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "BLENDING_PROC_INFO.json");
    // FILE *fp = fopen(dumpPath, "w");
    // if(NULL == fp) {
    //     MY_LOGE("Cannot dump BLENDING proc info to %s, error: %s", dumpPath, strerror(errno));
    //     return;
    // }

    // MY_LOGE_IF(fwrite(__dumpProcData, 1, strlen(__dumpProcData), fp) == 0, "Write failed");
    // MY_LOGE_IF(fflush(fp) != 0, "Flush failed");
    // MY_LOGE_IF(fclose(fp) != 0, "Close failed");
}

void
BLENDING_HAL_IMP::__dumpWorkingBuffer()
{
    if(!IS_DUMP_ENALED ||
       checkStereoProperty("vendor.STEREO.dump.blending_wb") < 1)
    {
        return;
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "BLENDING_WORKING_BUFFER");
    FILE *fp = fopen(dumpPath, "wb");
    if(NULL == fp) {
        MY_LOGE("Cannot dump BLENDING working buffer to %s, error: %s", dumpPath, strerror(errno));
        return;
    }

    MY_LOGE_IF(fwrite(__initInfo.workingBuffAddr, 1, __initInfo.workingBuffSize, fp) == 0, "Write failed");
    MY_LOGE_IF(fflush(fp) != 0, "Flush failed");
    MY_LOGE_IF(fclose(fp) != 0, "Close failed");
}
