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
#ifndef PASS2A_SIZE_PROVIDER_H_
#define PASS2A_SIZE_PROVIDER_H_

#include "pass2_size_provider_base.h"
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>

#include <algorithm>

class Pass2A_SizeProvider: public Pass2SizeProviderBase<Pass2A_SizeProvider>
{
public:
    friend class Pass2SizeProviderBase<Pass2A_SizeProvider>;

    virtual StereoArea getWDMAArea( P2SizeQueryParam param ) const {
        switch(param.eScenario) {
            case eSTEREO_SCENARIO_PREVIEW:
            case eSTEREO_SCENARIO_RECORD:
                {
                    MSize pvSize = StereoSizeProvider::getInstance()->getPreviewSize();
                    StereoArea area(pvSize);
                    int eisFactor = (int)StereoSettingProvider::getEISFactor();
                    if(eisFactor > 100)
                    {
                        //P2 constrain
                        pvSize.w = pvSize.w * eisFactor / 100;
                        StereoHAL::applyNAlign(8, pvSize.w);
                        pvSize.h = pvSize.h + pvSize.w - area.size.w;
                        StereoHAL::applyNAlign(2, pvSize.w);
                        area.size = pvSize;
                    }
                    else
                    {
                        area.apply8AlignToWidth(!IS_KEEP_CONTENT)    //P2 constrain
                            .apply2AlignToHeight(!IS_KEEP_CONTENT);
                    }

                    return area;
                }
                break;
            case eSTEREO_SCENARIO_CAPTURE:
                return StereoSizeProvider::getInstance()->captureImageSize();
                break;
            default:
                break;
        }

        return STEREO_AREA_ZERO;
    }

    virtual StereoArea getWROTArea( P2SizeQueryParam param ) const  {
        StereoArea area(960, 540);
        if(StereoSettingProvider::isDeNoise())
        {
            area.applyRatio(eStereoRatio_4_3);
        }
        else
        {
            area.applyRatio(StereoSettingProvider::imageRatio());
        }

        //If preview size is smaller than result, shrink to preview size
        MSize pvSize = StereoSizeProvider::getInstance()->getPreviewSize();
        if(area.size.w > pvSize.w ||
           area.size.h > pvSize.h)
        {
            area.size = pvSize;
        }

        area.rotatedByModule(param.withSlant)
            .apply16AlignRounding();

        return area;
    }

    virtual StereoArea getFEOInputArea( P2SizeQueryParam param __attribute__((unused)) ) const {
        return StereoArea(1600, 900)
               .applyRatio(StereoSettingProvider::imageRatio())
               .apply4Align();
    }

    virtual StereoArea getIMG2OArea( P2SizeQueryParam param __attribute__((unused)) ) const {
        return StereoArea(640, 360)
               .applyRatio(StereoSettingProvider::imageRatio(), E_KEEP_WIDTH)
               .apply4Align();
    }
protected:
    Pass2A_SizeProvider() {}
    virtual ~Pass2A_SizeProvider() {}
private:

};

#endif