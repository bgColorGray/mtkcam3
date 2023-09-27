/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2016. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
//
#define PIPE_MODULE_TAG "BokehNode"
#define PIPE_CLASS_TAG "ColorizedDepthUtil"
//
#include <featurePipe/core/include/PipeLog.h>
#include "StreamingFeatureNode.h"
#include "ColorizedDepthUtil.h"
//
CAM_ULOG_DECLARE_MODULE_ID(MOD_UTILITY);
//
using namespace NSCam;
using namespace NSCam::NSCamFeature::NSFeaturePipe;
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
//************************************************************************
//
//************************************************************************
double ColorizedDepthUtil::interpolate(double val, double y0, double x0, double y1, double x1) {
    return (val - x0)*(y1 - y0) / (x1 - x0) + y0;
}
double ColorizedDepthUtil::base(double val)
{
    if (val <= -0.75) return 0;
    else if (val <= -0.25) return interpolate(val, 0.0, -0.75, 1.0, -0.25);
    else if (val <= 0.25) return 1.0;
    else if (val <= 0.75) return interpolate(val, 1.0, 0.25, 0.0, 0.75);
    else return 0.0;
}
double ColorizedDepthUtil::R_jet(double gray) {
    return base(gray - 0.5);
}
double ColorizedDepthUtil::G_jet(double gray) {
    return base(gray);
}
double ColorizedDepthUtil::B_jet(double gray) {
    return base(gray + 0.5);
}
double ColorizedDepthUtil::blue(double grayscale) {
    if (grayscale < -0.33) return 1.0;
    else if (grayscale < 0.33) return interpolate(grayscale, 1.0, -0.33, 0.0, 0.33);
    else return 0.0;
}
double ColorizedDepthUtil::green(double grayscale) {
    if (grayscale < -1.0) return 0.0; // unexpected grayscale value
    if (grayscale < -0.33) return interpolate(grayscale, 0.0, -1.0, 1.0, -0.33);
    else if (grayscale < 0.33) return 1.0;
    else if (grayscale <= 1.0) return interpolate(grayscale, 1.0, 0.33, 0.0, 1.0);
    else return 1.0; // unexpected grayscale value
}
double ColorizedDepthUtil::red(double grayscale) {
    if (grayscale < -0.33) return 0.0;
    else if (grayscale < 0.33) return interpolate(grayscale, 0.0, -0.33, 1.0, 0.33);
    else return 1.0;
}
#define CLIP(min, max, a)  ((a > max) ? max : ((a < min) ? min : a))
MVOID
ColorizedDepthUtil::SetDispColor(char &y, char &u, char &v, char gray)
{
    char B = 255.0*B_jet(((double)gray - 128.0) / 128.0);
    char G = 255.0*G_jet(((double)gray - 128.0) / 128.0);
    char R = 255.0*R_jet(((double)gray - 128.0) / 128.0);
    y = CLIP ( 0.0, 255.0, 0.299 * (float)R + 0.587 * (float)G + 0.114 * (float)B );
    u = CLIP ( 0.0, 255.0, 0.511 * (float)B - 0.172 * (float)R - 0.339 * (float)G + 128.0 );
    v = CLIP ( 0.0, 255.0, 0.511 * (float)R - 0.428 * (float)G - 0.083 * (float)B + 128.0 );
    return;
}
MVOID
ColorizedDepthUtil::GenColorMappingTable()
{
    char y=0;
    char u=0;
    char v=0;
    int depth_hole = property_get_int32("demo.hole", 0);
    jet_y[depth_hole] = 0;
    jet_u[depth_hole] = 128;
    jet_v[depth_hole] = 128;
    for (int i=1;i<256;i++){
        SetDispColor(y, u, v, i);
        jet_y[i] = y;
        jet_u[i] = u;
        jet_v[i] = v;
    }
}
MVOID
ColorizedDepthUtil::genDepthMappingTable() {
    int depth_near = property_get_int32("demo.near", 180);
    int depth_far = property_get_int32("demo.far", 688);
    int fb = property_get_int32("demo.fb", 68474);
    int min_depth = 1;
    int max_depth = 255;
    memset(disp2depth, 0, sizeof(char)*512);
    for (int i = 255; i >= 8; i--) {
        float depth = fb / (float)i;
        if (depth < depth_near) {
        disp2depth[i] = (char)min_depth;
        }
        else {
            if ( (depth_far - depth_near) == 0 )
            {
                return;
            }
            disp2depth[i] = CLIP(min_depth, max_depth, ((depth - depth_near) * 255) / (depth_far - depth_near));
        }
    }
    for (int i = 7; i >= 0; i--) {
        disp2depth[i] = disp2depth[8] + ((float)(8 - i) / 8)*(max_depth - disp2depth[8]);
    }
}
MVOID
ColorizedDepthUtil::overlapDisplay(
    IImageBuffer* depth,
    IImageBuffer* displayResult
)
{
    if(depth == nullptr || displayResult == nullptr)
        return;
    MSize outImgSize = displayResult->getImgSize();
    MSize inImgSize = depth->getImgSize();
    char* outAddr0 = (char*)displayResult->getBufVA(0);
    char* outAddr1 = (char*)displayResult->getBufVA(1);
    // char* outAddr2 = (char*)displayResult->getBufVA(2);
    char* inAddr = (char*)depth->getBufVA(0);
    MINT32 halfInWidth = inImgSize.w >> 1;
    MINT32 halfInHeight = inImgSize.h >> 1;
    MINT32 halfOutWidth = outImgSize.w  >> 1;
    //
    int depth_color = property_get_int32("vendor.debug.demo.color", 1);
    int showdepth = property_get_int32("debug.vsdof.showdepthmap", SHOW_DEPTH_DEFAULT);
    if (depth_color) {
        for(int j=0;j<inImgSize.h;j++){
            for(int i=0;i<inImgSize.w;i++){
                if(depth->getImgFormat() == eImgFmt_Y16) {
                    MUINT16 *data = (MUINT16 *)inAddr;
                    MUINT16 gray = *data;
                    if (gray) {
                        if (gray > 11008) gray = 255; // 688 -> 255
                        else if (gray < 2880) gray = 1; // 180 -> 1
                        else {
                            gray = (gray >> 5) - 89; // [1,255]
                        }
                    }
                    outAddr0[i + j*outImgSize.w] = jet_y[gray];
                    if(i%2 == 0 && j%2 == 0){
                        int i_uv = i >> 1;
                        int j_uv = j >> 1;
                        outAddr1[i_uv + j_uv*halfOutWidth] = jet_u[gray];
                        // outAddr2[i_uv + j_uv*halfOutWidth] = jet_v[gray];
                    }
                    inAddr += 2;
                }
                else {
                    unsigned int gray = inAddr[i + j*inImgSize.w];
                    if (showdepth == 2 || showdepth == 3){
                        gray = disp2depth[gray];
                    }
                    outAddr0[i + j*outImgSize.w] = jet_y[gray];
                    if(i%2 == 0 && j%2 == 0){
                        int i_uv = i >> 1;
                        int j_uv = j >> 1;
                        outAddr1[i_uv + j_uv*halfOutWidth] = jet_u[gray];
                        // outAddr2[i_uv + j_uv*halfOutWidth] = jet_v[gray];
                    }
                }
            }
        }
    }
    else {
        for(int i=0;i<inImgSize.h;++i)
        {
            if(depth->getImgFormat() == eImgFmt_Y16)
            {
                MUINT8 *target = (MUINT8*) outAddr0;
                MUINT16 *data = (MUINT16 *)inAddr;
                for(int j=0;j<inImgSize.w;j++)
                {
                    *target = (MUINT8) ((*data) >> 4);
                    target++;
                    data++;
                }
                inAddr += (inImgSize.w * 2); // for Y16 need *2
            }
            else
            {
                memcpy(outAddr0, inAddr, inImgSize.w);
                inAddr += inImgSize.w;
            }
            outAddr0 += outImgSize.w;
        }
        //
        for(int i=0;i<halfInHeight;++i)
        {
            memset(outAddr1, 128, halfInWidth);
            // memset(outAddr2, 128, halfInWidth);
            outAddr1 += halfOutWidth;
            // outAddr2 += halfOutWidth;
        }
    }
    return;
}
//************************************************************************
//
//************************************************************************
ColorizedDepthUtil::
ColorizedDepthUtil()
{
    jet_y = new char[512];
    jet_u = new char[512];
    jet_v = new char[512];
    disp2depth = new char[512];
    GenColorMappingTable();
    genDepthMappingTable();
}
//************************************************************************
//
//************************************************************************
ColorizedDepthUtil::
~ColorizedDepthUtil()
{
    delete jet_y;
    delete jet_u;
    delete jet_v;
    delete disp2depth;
}
};
};
};