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
#ifndef _GF_HAL_H_
#define _GF_HAL_H_

#include <mtkcam3/feature/stereo/hal/stereo_common.h>
#include <vector>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <faces.h>  //for MtkCameraFaceMetadata

namespace StereoHAL {

struct GF_HAL_INIT_DATA
{
      ENUM_STEREO_SCENARIO  eScenario            = eSTEREO_SCENARIO_PREVIEW;
      float                 focalLenght          = 0.0f;    // mm
      float                 aperture             = 0.0f;    // f
      float                 sensorPhysicalWidth  = 0.0f;    // mm
      float                 sensorPhysicalHeight = 0.0f;    // mm

      bool                  outputDepthmap       = false;
};

struct GF_HAL_IN_DATA
{
    int     magicNumber                = 0;
    int     requestNumber              = 0;
    ENUM_STEREO_SCENARIO scenario      = eSTEREO_SCENARIO_UNKNOWN;

    MINT32  dofLevel                   = 0;

    // For bokeh
    NSCam::IImageBuffer *depthMap      = NULL;  //DMW, hole filled
    std::vector<NSCam::IImageBuffer *> images;

    // For GF AF
    NSCam::IImageBuffer *confidenceMap = NULL;
    NSCam::IImageBuffer *nocMap        = NULL;
    NSCam::IImageBuffer *pdMap         = NULL;

    MFLOAT  convOffset                 = 0.0f; //from N3D HAL

    MINT32  touchPosX                  = 0;
    MINT32  touchPosY                  = 0;

    bool    isCapture                  = false;

    bool    hasFEFM                    = false;

    float   focalLensFactor            = 0.0f;

    MtkCameraFaceMetadata *fdData      = NULL;

    float   previewCropRatio           = 1.0f;

    NSCam::TuningUtils::FILE_DUMP_NAMING_HINT *dumpHint = NULL;
    MINT64  ndd_timestamp              = 0;
};

enum ENUM_GF_WARNING
{
    E_GF_WARNING_NONE,
    E_GF_WARNING_TOO_CLOSE,
    E_GF_WARNING_TOO_FAR,
};

struct GF_HAL_OUT_DATA
{
    NSCam::IImageBuffer *dmbg     = NULL;
    NSCam::IImageBuffer *depthMap = NULL;   //Output size is configured in camera_custom_stereo.cpp of each target

    int                 warning   = 0;
};

class GF_HAL
{
public:

    /**
     * \brief Create a new GF instance(not singleton)
     * \details Callers should delete the instance by themselves:
     *          std::shared_ptr<GF_HAL> gfHal = GF_HAL::createInstance();
     *          ...
     *          gfHal = nullptr;
     *
     * \param data Init data
     * \param outputDepthmap Set to true to output depthmap instead of DMBG
     *
     * \return Pointer to GF HAL instance(not singleton)
     */
    static std::shared_ptr<GF_HAL> createInstance(GF_HAL_INIT_DATA &initData);

    /**
     * \brief Create a new GF instance(not singleton)
     * \details Callers should delete the instance by themselves:
     *          std::shared_ptr<GF_HAL> gfHal = GF_HAL::createInstance();
     *          ...
     *          gfHal = nullptr;
     *
     * \param eScenario Scenario to use GF
     * \param outputDepthmap Set to true to output depthmap instead of DMBG
     *
     * \return Pointer to GF HAL instance(not singleton)
     */
    static std::shared_ptr<GF_HAL> createInstance(ENUM_STEREO_SCENARIO eScenario, bool outputDepthmap=false);

    /**
     * \brief Default destructor
     * \details Callers should delete the instance by themselves
     */
    virtual ~GF_HAL() {}

    /**
     * \brief Run GF
     * \details Run GF algorithm and get result
     *
     * \param inData Input data
     * \param outData Output data
     *
     * \return True if success
     */
    virtual bool GFHALRun(GF_HAL_IN_DATA &inData, GF_HAL_OUT_DATA &outData) = 0;

    /**
     * \brief Run GF AF
     * \details GF AF will
     *
     * \param inData [description]
     * \return [description]
     */
    virtual bool GFAFRun(GF_HAL_IN_DATA &inData);
    //
protected:
    GF_HAL() {}
};

};  //namespace StereoHAL
#endif