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
#ifndef PASS2A_CROP_SIZE_PROVIDER_H_
#define PASS2A_CROP_SIZE_PROVIDER_H_

#include "pass2_size_provider_base.h"
#include <stereo_crop_util.h>

class Pass2A_Crop_SizeProvider: public Pass2SizeProviderBase<Pass2A_Crop_SizeProvider>
{
public:
    friend class Pass2SizeProviderBase<Pass2A_Crop_SizeProvider>;

    virtual StereoArea getWDMAArea( P2SizeQueryParam param ) const  {
        MUINT32 junkStride;
        MSize   szMain1Output;
        MRect   tgCropRect;

        StereoArea area;
        //Only capture(IMGO) shuold be cropped by P2, preview/record(RRZO) is pre-cropped by P1
        if( !StereoSettingProvider::isDeNoise() &&
            eSTEREO_SCENARIO_CAPTURE == param.eScenario )
        {
            StereoSizeProvider::getInstance()->
                getPass1Size( { eSTEREO_SENSOR_MAIN1, eImgFmt_BAYER10,
                                EPortIndex_IMGO, param.eScenario, 1.0f },
                              //below are outputs
                              tgCropRect,
                              szMain1Output,
                              junkStride );

            auto &imageRatio = StereoSettingProvider::imageRatio();
            area = szMain1Output;
            CropUtil::cropStereoAreaByImageRatio(area, imageRatio);
            if(!StereoSettingProvider::is3rdParty()) {
                CropUtil::cropStereoAreaByFOV(eSTEREO_SENSOR_MAIN1, area,
                                              StereoSettingProvider::getMain1FOVCropRatio(),
                                              imageRatio);
            }
        } else {
            MRect tgCropRect1X;
            StereoSizeProvider::getInstance()->
                getPass1Size( { eSTEREO_SENSOR_MAIN1, eImgFmt_FG_BAYER10,
                                EPortIndex_RRZO, param.eScenario, 1.0f },
                              //below are outputs
                              tgCropRect1X,
                              szMain1Output,
                              junkStride );
            area = szMain1Output;

            if(param.previewCropRatio < 1.0f) {
                StereoSizeProvider::getInstance()->
                    getPass1Size( { eSTEREO_SENSOR_MAIN1, eImgFmt_FG_BAYER10,
                                    EPortIndex_RRZO, param.eScenario, param.previewCropRatio },
                                  //below are outputs
                                  tgCropRect,
                                  szMain1Output,
                                  junkStride );

                float p1Scale = static_cast<float>(tgCropRect.s.w)/tgCropRect1X.s.w;
                if(p1Scale > param.previewCropRatio) {
                    // If P1 does not crop enough, then P2 has to handle the rest
                    float p2Scale = param.previewCropRatio / p1Scale;
                    szMain1Output.w *= p2Scale;
                    szMain1Output.h *= p2Scale;
                    area.padding = area.size - szMain1Output;
                    area.startPt.x = area.padding.w/2;
                    area.startPt.y = area.padding.h/2;
                }
            }
        }

        return area.apply2AlignToContent();
    }

    virtual StereoArea getWROTArea( P2SizeQueryParam param ) const  {
        //This is the size before module rotation, so we don't apply rotatedByModule
        return getWDMAArea(param);
    }

    virtual StereoArea getIMG2OArea( P2SizeQueryParam param ) const  {
        //This is the size before module rotation, so we don't apply rotatedByModule
        return getWDMAArea(param);
    }

    virtual StereoArea getIMG3OArea( P2SizeQueryParam param ) const {
        //This is the size before module rotation, so we don't apply rotatedByModule
        return getWDMAArea(param);
    }

    virtual StereoArea getFEOInputArea( P2SizeQueryParam param ) const {
        //This is the size before module rotation, so we don't apply rotatedByModule
        return getWDMAArea(param);
    }

protected:
    Pass2A_Crop_SizeProvider() {}
    virtual ~Pass2A_Crop_SizeProvider() {}
private:

};

#endif