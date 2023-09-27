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
#ifndef PASS2A_P_3_SIZE_PROVIDER_H_
#define PASS2A_P_3_SIZE_PROVIDER_H_

#include "pass2_size_provider_base.h"
#include "pass2A_3_size_provider.h"

class Pass2A_P_3_SizeProvider: public Pass2SizeProviderBase<Pass2A_P_3_SizeProvider>
{
public:
    friend class Pass2SizeProviderBase<Pass2A_P_3_SizeProvider>;

    virtual StereoArea getIMG2OArea( P2SizeQueryParam param ) const {
        StereoArea area = Pass2A_3_SizeProvider::instance()->getIMG2OArea(param);
        const float FOV_RATIO = StereoSettingProvider::getStereoCameraFOVRatio(param.previewCropRatio);
        if(FOV_RATIO > 1.0f) {
            area *= FOV_RATIO;
        }
        return area.apply4Align();
    }

    virtual StereoArea getFEOInputArea( P2SizeQueryParam param ) const {
        StereoArea p2a3FEOArea = Pass2A_3_SizeProvider::instance()->getFEOInputArea(param);
        if(!StereoSettingProvider::isSlantCameraModule())
        {
            MSize oriFEOSize = p2a3FEOArea.contentSize();
            StereoArea areaFE(Pass2A_P_2_SizeProvider::instance()->getIMG2OArea(param.withoutCrop()));
            areaFE.apply8Align();
            areaFE.padding = areaFE.size - oriFEOSize;
            areaFE.startPt = MPoint(areaFE.padding.w>>1, areaFE.padding.h>>1);
            return areaFE;
        } else if(!param.withSlant) {
            auto areaFE = Pass2A_P_2_SizeProvider::instance()->getFEOInputArea(param);
            return areaFE * 0.5f;
        } else {
            return p2a3FEOArea;
        }

    }

protected:
    Pass2A_P_3_SizeProvider() {}
    virtual ~Pass2A_P_3_SizeProvider() {}
private:

};

#endif