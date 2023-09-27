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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#define LOG_TAG "MtkCam/P1NodeImp"
//

#include "P1NodeImageSteramInfo.h"
#include "P1TaskCtrl.h"
#include "P1NodeImp.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace std;
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::NSP1Node;
using namespace NSCam::Utils::Sync;
using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSCamIOPipe;
using namespace NS3Av3;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
namespace NSCam {
namespace v3 {
namespace NSP1Node {

const SttInfo *
P1NodeImageStreamInfo::
getSttInfo()
{
    const SttInfo *pStt = NULL;
    QueryData data;
    bool exist;

    if(streamImg != STREAM_IMG_OUT_LCS && streamImg != STREAM_IMG_OUT_LCESHO)
    {
        return NULL;
    }

    data = queryPrivateData();
    exist = NSCam::plugin::streaminfo::has_data(data.privData);
    pStt = exist ? NSCam::plugin::streaminfo::get_data_if<SttInfo>(data.privData) : NULL;
    if(pStt != NULL)
    {
        switch(streamImg) {
        case STREAM_IMG_OUT_LCS:
            if(!pStt->useLcso)
                pStt = NULL;
            break;
        case STREAM_IMG_OUT_LCESHO:
            if(!pStt->useLcsho)
                pStt = NULL;
            break;
        default:
            pStt = NULL;
        }
    }
    return pStt;
}

MBOOL
P1NodeImageStreamInfo::
isSttExist()
{
    const SttInfo *pStt = getSttInfo();
    return pStt ? MTRUE : MFALSE;
};

const ImageBufferInfo *
P1NodeImageStreamInfo::
getSttInfoPtr()
{
    const SttInfo *pStt = getSttInfo();

    if(pStt) {
        switch(streamImg) {
        case STREAM_IMG_OUT_LCS:
            return &pStt->mLcsoInfo;

        case STREAM_IMG_OUT_LCESHO:
            return &pStt->mLcshoInfo;
        default:
            break;
        }
        return NULL;
    }
    return NULL;
}

MINT
P1NodeImageStreamInfo::
getImgFormat()
{
    const ImageBufferInfo* pSttInfo = getSttInfoPtr();

    if(pSttInfo)
        return pSttInfo->imgFormat;

    return info->getImgFormat();
}

IImageStreamInfo::BufPlanes_t const&
P1NodeImageStreamInfo::
getBufPlanes()
{
    const ImageBufferInfo* pSttInfo = getSttInfoPtr();
    if(pSttInfo)
        return pSttInfo->bufPlanes;

    return info->getBufPlanes();
}
MSize
P1NodeImageStreamInfo::
getImgSize()
{
    const ImageBufferInfo* pSttInfo = getSttInfoPtr();
    if(pSttInfo)
        return MSize(pSttInfo->imgWidth, pSttInfo->imgHeight);

    return info->getImgSize();
}

ImageBufferInfo const&
P1NodeImageStreamInfo::
getImageBufferInfo()
{
    const ImageBufferInfo* pSttInfo = getSttInfoPtr();
    if(pSttInfo)
        return *pSttInfo;

    return info->getImageBufferInfo();
}


};//namespace NSP1Node
};//namespace v3
};//namespace NSCam


