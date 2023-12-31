/*********************************************************************************************
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
#include <mtkcam/utils/std/ULog.h>
#define CAM_ULOG_MODULE_ID MOD_VSDOF_HAL
#include "n3d_hal_imp.h"         // For N3D_HAL class.
using namespace StereoHAL;

void
N3D_HAL_IMP::__dumpNVRAM(bool isInput)
{
    if( !IS_DUMP_ENALED ||
        NULL == __spVoidGeoData )
    {
        return;
    }

    string bufferName;
    char dumpPath[PATH_MAX] = {0};
    if(isInput) {
        genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "N3D_NVRAM_IN");
    } else if(__dumpLevel > 1) {
        genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "N3D_NVRAM_OUT");

        // Extract NVRAM content from N3D
        __n3dKernel.updateNVRAM(__spVoidGeoData);
    }

    if(dumpPath[0] != 0) {
        FILE *fpNVRAM = NULL;
        fpNVRAM = fopen(dumpPath, "wb");
        if(fpNVRAM) {
            MY_LOGE_IF(fwrite(__spVoidGeoData->StereoNvramData.StereoData, 1, sizeof(MUINT32)*MTK_STEREO_KERNEL_NVRAM_LENGTH, fpNVRAM) == 0, "Write failed");
            MY_LOGE_IF(fflush(fpNVRAM), "Flush failed");
            MY_LOGE_IF(fclose(fpNVRAM), "Close failed");
            fpNVRAM = NULL;
        } else {
            MY_LOGE("Cannot dump NVRAM to %s, error: %s", dumpPath, strerror(errno));
        }
    }
}

void
N3D_HAL_IMP::__dumpLDC()
{
    if(!StereoSettingProvider::LDCEnabled() ||
       StereoSettingProvider::getLDCTable().size() <= 1 ||
       !IS_DUMP_ENALED)
    {
        return;
    }

    if(__ldcString.size() < 1) {
        //Prepare dump string
        float *element = &(StereoSettingProvider::getLDCTable()[0]);
        std::ostringstream oss;
        oss.precision(10);
        oss << std::fixed;

        for(int k = 0; k < 2; k++) {
            int line = (int)*element++;
            int sizePerLine = (int)*element++;
            oss << line << " " << sizePerLine << endl;

            for(int row = 1; row <= line; row++) {
                for(int col = 0; col < sizePerLine; col++) {
                    oss << *element++ << " ";
                }
                oss << (int)*element++ << " ";
                oss << (int)*element++ << endl;
            }
        }

        __ldcString = oss.str();
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "N3D_LDC");
    FILE *fp = fopen(dumpPath, "wb");
    if(fp) {
        MY_LOGE_IF(fwrite(__ldcString.c_str(), 1, __ldcString.size(), fp) == 0, "Write failed");
        MY_LOGE_IF(fflush(fp), "Flush failed");
        MY_LOGE_IF(fclose(fp), "Close failed");
    } else {
        MY_LOGE("Cannot open %s", dumpPath);
    }
}

void
N3D_HAL_IMP::__dumpN3DExtraData()
{
    if(__stereoExtraData.size() < 2 ||
       checkStereoProperty(PROPERTY_DUMP_JSON) != 1 ||
       !IS_DUMP_ENALED ||
       __dumpLevel < 2)
    {
        return;
    }

    char dumpPath[PATH_MAX];
    genFileName_VSDOF_BUFFER(dumpPath, PATH_MAX, __dumpHint, "N3D_EXTRA_DATA");
    FILE *fp = fopen(dumpPath, "wb");
    if(fp) {
        MY_LOGE_IF(fwrite(__stereoExtraData.c_str(), 1, __stereoExtraData.size(), fp) == 0, "Write failed");
        MY_LOGE_IF(fflush(fp), "Flush failed");
        MY_LOGE_IF(fclose(fp), "Close failed");
    } else {
        MY_LOGE("Cannot open %s", dumpPath);
    }
}
