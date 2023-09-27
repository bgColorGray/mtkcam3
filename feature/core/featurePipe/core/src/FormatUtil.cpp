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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#include "../include/FormatUtil.h"

#include "../include/DebugControl.h"
#define PIPE_TRACE TRACE_FORMAT_UTIL
#define PIPE_CLASS_TAG "FormatUtil"
#include "../include/PipeLog.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_COMMON);

#include <unordered_map>
#include <vector>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

class MapImgFmts
{
protected:
    MapImgFmts()
    {
        size_t tableSize = mImgFmtsTable.size();
        mFmtToName.reserve(tableSize);
        mNameToFmt.reserve(tableSize);
        for(const FmtName &imgFmt : mImgFmtsTable)
        {
            mFmtToName.insert(std::pair<EImageFormat, std::string>(imgFmt.fmt, imgFmt.name));
            mNameToFmt.insert(std::pair<std::string, EImageFormat>(imgFmt.name, imgFmt.fmt));
        }
    }

public:
    static const char* getFormatName(const EImageFormat &fmt)
    {
        const MapImgFmts &imgFmt = getMapFmt();
        const auto iter = imgFmt.mFmtToName.find(fmt);
        return (iter == imgFmt.mFmtToName.end()) ? "UNKNOW" : iter->second.c_str();
    }
    static EImageFormat getEImageFormat(const std::string &name)
    {
        const MapImgFmts &imgFmt = getMapFmt();
        const auto iter = imgFmt.mNameToFmt.find(name);
        return (iter == imgFmt.mNameToFmt.end()) ? eImgFmt_UNKNOWN : iter->second;
    }
private:
    static const MapImgFmts& getMapFmt()
    {
      static MapImgFmts sMapImgFmts;
      return sMapImgFmts;
    }

private:
    std::unordered_map<EImageFormat, std::string> mFmtToName;
    std::unordered_map<std::string, EImageFormat> mNameToFmt;

    struct FmtName
    {
        EImageFormat fmt = eImgFmt_UNKNOWN;
        std::string name;
    };
    std::vector<FmtName> mImgFmtsTable = {
        {eImgFmt_BLOB, "BLOB"},
        {eImgFmt_RGBA8888, "RGBA8888"},
        {eImgFmt_RGBX8888, "RGBX8888"},
        {eImgFmt_RGB888, "RGB888"},
        {eImgFmt_RGB565, "RGB565"},
        {eImgFmt_BGRA8888, "BGRA8888"},
        {eImgFmt_YUY2, "YUY2"},
        {eImgFmt_NV16, "NV16"},
        {eImgFmt_NV21, "NV21"},
        {eImgFmt_NV12, "NV12"},
        {eImgFmt_YV12, "YV12"},
        {eImgFmt_Y8, "Y8"},
        //{eImgFmt_Y800, "Y800"},
        {eImgFmt_Y16, "Y16"},
        {eImgFmt_CAMERA_OPAQUE, "opaque"},
        {eImgFmt_YVYU, "YVYU"},
        {eImgFmt_UYVY, "UYVY"},
        {eImgFmt_VYUY, "VYUY"},
        {eImgFmt_NV61, "NV61"},
        {eImgFmt_NV12_BLK, "NV12_BLK"},
        {eImgFmt_NV21_BLK, "NV21_BLK"},
        {eImgFmt_YV16, "YV16"},
        {eImgFmt_I420, "I420"},
        {eImgFmt_I422, "I422"},
        {eImgFmt_YUYV_Y210, "YUYV_Y210"},
        {eImgFmt_YVYU_Y210, "YVYU_Y210"},
        {eImgFmt_UYVY_Y210, "UYVY_Y210"},
        {eImgFmt_VYUY_Y210, "VYUY_Y210"},
        {eImgFmt_YUV_P210, "YUV_P210"},
        {eImgFmt_YVU_P210, "YVU_P210"},
        {eImgFmt_YUV_P210_3PLANE, "YVU_P210_3P"},
        {eImgFmt_YUV_P010, "YUV_P010"},
        {eImgFmt_YVU_P010, "YVU_P010"},
        {eImgFmt_YUV_P010_3PLANE, "YUV_P010_3P"},
        {eImgFmt_MTK_YUYV_Y210, "MTK_YUYV_Y210"},
        {eImgFmt_MTK_YVYU_Y210, "MTK_YVYU_Y210"},
        {eImgFmt_MTK_UYVY_Y210, "MTK_UYVY_Y210"},
        {eImgFmt_MTK_VYUY_Y210, "MTK_VYUY_Y210"},
        {eImgFmt_MTK_YUV_P210, "MTK_YUV_P210"},
        {eImgFmt_MTK_YVU_P210, "MTK_YVU_P210"},
        {eImgFmt_MTK_YUV_P210_3PLANE, "MTK_YVU_P210_3P"},
        {eImgFmt_MTK_YUV_P010, "MTK_YUV_P010"},
        {eImgFmt_MTK_YVU_P010, "MTK_YVU_P010"},
        {eImgFmt_MTK_YUV_P010_3PLANE, "MTK_YUV_P010_3P"},
        {eImgFmt_YUV_P012, "YUV_P012"},
        {eImgFmt_YVU_P012, "YVU_P012"},
        {eImgFmt_MTK_YUV_P012, "MTK_YUV_P012"},
        {eImgFmt_MTK_YVU_P012, "MTK_YVU_P012"},
        {eImgFmt_YUFD_NV12, "YUFD_NV12"},
        {eImgFmt_YUFD_NV21, "YUFD_NV21"},
        {eImgFmt_YUFD_MTK_YUV_P010, "YUFD_MTK_YUV_P010"},
        {eImgFmt_YUFD_MTK_YVU_P010, "YUFD_MTK_YVU_P010"},
        {eImgFmt_UFBC_NV12, "UFBC_NV12"},
        {eImgFmt_UFBC_NV21, "UFBC_NV21"},
        {eImgFmt_UFBC_MTK_YUV_P010, "UFBC_MTK_YUV_P010"},
        {eImgFmt_UFBC_MTK_YVU_P010, "UFBC_MTK_YVU_P010"},
        {eImgFmt_ARGB8888, "ARGB8888"},
        //{eImgFmt_ARGB888, "ARGB888"},
        {eImgFmt_RGB48, "RGB48"},
        {eImgFmt_BAYER8, "BAYER8"},
        {eImgFmt_BAYER10, "BAYER10"},
        {eImgFmt_BAYER12, "BAYER12"},
        {eImgFmt_BAYER14, "BAYER14"},
        {eImgFmt_FG_BAYER8, "FG_BAYER8"},
        {eImgFmt_FG_BAYER10, "FG_BAYER10"},
        {eImgFmt_FG_BAYER12, "FG_BAYER12"},
        {eImgFmt_FG_BAYER14, "FG_BAYER14"},
        {eImgFmt_WARP_1PLANE, "WARP_1P"},
        {eImgFmt_WARP_2PLANE, "WARP_2P"},
        {eImgFmt_WARP_3PLANE, "WARP_3P"},
        {eImgFmt_STA_BYTE, "BYTE"},
        {eImgFmt_STA_2BYTE, "2BYTE"},
        {eImgFmt_STA_4BYTE, "4BYTE"},
        {eImgFmt_STA_10BIT, "10BIT"},
        {eImgFmt_STA_12BIT, "12BIT"},
    };
} ;

const char* toName(const EImageFormat &fmt)
{
    return MapImgFmts::getFormatName(fmt);
}

EImageFormat toEImageFormat(const char* name)
{
    return MapImgFmts::getEImageFormat(name ? name : "UNKNOW");
}

}; // namespace NSFeaturePipe
}; // namespace NSCamFeature
}; // namespace NSCam
