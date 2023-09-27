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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include <cutils/properties.h>
#include "P2_StreamingProcessor.h"

#include "P2_DebugControl.h"
#define P2_CLASS_TAG    Streaming_Draw
#define P2_TRACE        TRACE_STREAMING_DRAW
#include "P2_LogHeader.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_P2_STR_PROC);

namespace P2
{

enum DrawType
{
    DRAW_TYPE_LINE_NONE,
    DRAW_TYPE_LINE_V_MOVE,
    DRAW_TYPE_LINE_V_FIX,
    DRAW_TYPE_LINE_H_MOVE,
    DRAW_TYPE_LINE_H_FIX,
};

enum DrawTypeMask
{
    DRAW_TYPE_MASK_EN       = 1 << 0,  // enable draw
    DRAW_TYPE_MASK_IO_IN    = 1 << 1,  // input or output
    DRAW_TYPE_MASK_ORI_VER  = 1 << 2,  // orientation vertical or horizontal
    DRAW_TYPE_MASK_MOVE     = 1 << 3,  // move line or fixed line
};
#define DRAW_TYPE_EN(x)      ((x) & (DRAW_TYPE_MASK_EN))
#define DRAW_TYPE_IO_IN(x)   ((x) & (DRAW_TYPE_MASK_IO_IN))
#define DRAW_TYPE_IO_OUT(x)  (!DRAW_TYPE_IO_IN(x))
#define DRAW_TYPE_ORI_VER(x) ((x) & (DRAW_TYPE_MASK_ORI_VER))
#define DRAW_TYPE_MOVE(x)    ((x) & (DRAW_TYPE_MASK_MOVE))

enum Orientation
{
    ORI_V,
    ORI_H,
};

static MVOID drawLine(IImageBuffer* buffer, MUINT32 lineWidthPercent, MUINT32 value, Orientation ori, MUINT32 positionPercent)
{
    if( buffer != NULL )
    {
        buffer->syncCache(NSCam::eCACHECTRL_INVALID);
        const MUINT32 planeCount = buffer->getPlaneCount();
        for( MUINT32 plane = 0 ; plane < planeCount ; ++plane )
        {
            char* virtAddr = (char*)( buffer->getBufVA(plane) );
            MUINT32 stride = buffer->getBufStridesInBytes(plane);
            MUINT32 scanline = buffer->getBufScanlines(plane);

            MUINT32 moveScope = ( ori == ORI_V ) ? scanline : stride;
            MUINT32 move = (moveScope * positionPercent * 0.01);

            MUINT32 lineWidth = ( ori == ORI_V ) ? (scanline * lineWidthPercent * 0.01) : (stride * lineWidthPercent * 0.01);
            char* start_x = ( ori == ORI_V ) ? virtAddr : (virtAddr + move);
            MUINT32 start_y = ( ori == ORI_V ) ? move : 0;
            MUINT32 dx = ( ori == ORI_V ) ? stride : ( (move + lineWidth) <= stride ) ? lineWidth : (stride - move);
            MUINT32 dy = ( ori == ORI_V ) ? lineWidth : scanline;
            for( MUINT32 y = start_y ; y < (start_y + dy) && y < scanline ; y++ )
            {
                char* offset = start_x + y*stride;
                memset((void*)offset, value, dx);
            }
            MY_LOGD("plane[%u/%u] Fmt:%d, virtAddr:%p, stride:%u, scanline:%u, moveScope:%u, position:%u, move:%u, start_x_y(%p,%u), dx_dy(%u,%u)",
                plane, planeCount, buffer->getImgFormat(), virtAddr, stride, scanline, moveScope, positionPercent, move, start_x, start_y, dx, dy);
        }
        buffer->syncCache(NSCam::eCACHECTRL_FLUSH);
    }
    else MY_LOGW("ImageBuffer is NULL, can't draw it!");
}

MVOID StreamingProcessor::drawLine(const sp<P2Img> &img) const
{
    if( isValid(img) )
    {
        IImageBuffer *imgBuf = img->getIImageBufferPtr();
        if( imgBuf != NULL )
        {
            MUINT32 position = DRAW_TYPE_MOVE(mDrawType) ? ((img->getMagic3A()*mDrawMoveSpeed) % 100) : mDrawDefaultPosition;
            Orientation ori = DRAW_TYPE_ORI_VER(mDrawType) ? ORI_V : ORI_H;
            ::P2::drawLine(imgBuf, mDrawLineWidth, mDrawValue, ori, position);
        }
    }
    else
    {
        MY_LOGW("Not valid img!");
    }
}

MVOID StreamingProcessor::drawP2InputLine(const sp<Payload> &payload) const
{
    if( DRAW_TYPE_EN(mDrawType) && DRAW_TYPE_IO_IN(mDrawType) )
    {
        for( auto &partialPayload : payload->mPartialPayloads )
        {
            for( const auto &sensorID : mP2Info.getConfigInfo().mAllSensorID )
            {
                auto &reqPack = partialPayload->mRequestPack;
                auto &inputs = reqPack->mInputs;
                const auto &in = inputs[reqPack->mSensorInputMap[sensorID]];
                drawLine(in.mIMGI);
            }
        }
    }
}

MVOID StreamingProcessor::drawP2OutputLine(const sp<Payload> &payload) const
{
    if( DRAW_TYPE_EN(mDrawType) && DRAW_TYPE_IO_OUT(mDrawType) )
    {
        for( auto &partialPayload : payload->mPartialPayloads )
        {
            auto &reqPack = partialPayload->mRequestPack;
            for( auto &it : reqPack->mOutputs )
            {
                for( auto &out : it.second )
                {
                    if( out.isDisplay() || out.isRecord() )
                        drawLine(out.mImg);
                }
            }
        }
    }
}


}; // namespace P2
