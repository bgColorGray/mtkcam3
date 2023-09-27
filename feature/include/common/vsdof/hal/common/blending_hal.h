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
#ifndef _BLENDING_HAL_H_
#define _BLENDING_HAL_H_

#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <vector>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>

namespace StereoHAL {

struct BLENDING_HAL_IN_DATA
{
    MINT32  magicNumber                = -1;
    MINT32  requestNumber              = -1;
    ENUM_STEREO_SCENARIO scenario      = eSTEREO_SCENARIO_UNKNOWN;

    NSCam::IImageBuffer *cleanImage    = NULL;
    NSCam::IImageBuffer *bokehImage    = NULL;  //From HW bokeh
    NSCam::IImageBuffer *blurMap       = NULL;  //blur map
    std::vector<MUINT8> alphaTable; //Comes from GF, if not given, use offline tuning table

    NSCam::TuningUtils::FILE_DUMP_NAMING_HINT *dumpHint = NULL;
    int dumpLevel = 2;
};

struct BLENDING_HAL_OUT_DATA
{
    NSCam::IImageBuffer *bokehImage = NULL;   //Output size is configured in camera_custom_stereo.cpp of each target
};

class BLENDING_HAL
{
public:
    /**
     * \brief Create a new blending instance(not singleton)
     * \details Callers should delete the instance by themselves:
     *          BLENDING_HAL *pHal = BLENDING_HAL::createInstance();
     *          ...
     *          if(pHal) delete pHal;
     *
     * \param eScenario Scenario to use blending
     *
     * \return Pointer to blending HAL instance(not singleton)
     */
    static BLENDING_HAL *createInstance(ENUM_STEREO_SCENARIO eScenario);

    /**
     * \brief Default destructor
     * \details Callers should delete the instance by themselves
     */
    virtual ~BLENDING_HAL() {}

    /**
     * \brief Run blending
     * \details Run blending algorithm and get result
     *
     * \param inData Input data
     * \param outData Output data
     *
     * \return True if success
     */
    virtual bool BlendingHALRun(BLENDING_HAL_IN_DATA &inData, BLENDING_HAL_OUT_DATA &outData) = 0;
    //
protected:
    BLENDING_HAL() {}
};

};  //namespace StereoHAL
#endif