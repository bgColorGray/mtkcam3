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
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_IMFNRCapability_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_IMFNRCapability_H_

#include <stdlib.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>

#include <mtkcam/utils/hw/FleetingQueue.h>

#include "MFNRShotInfo.h"


using namespace android;
using namespace mfll;

namespace NSCam {
namespace plugin {

/******************************************************************************
*
******************************************************************************/

class IMFNRCapability
{
    typedef NSPipelinePlugin::MultiFramePlugin::Selection Selection;

// Constructor & destructor
//
public:
    virtual ~IMFNRCapability() {};

public:
    virtual void setShotInfo(std::shared_ptr<MFNRShotInfo> pShotinfo) = 0;

    virtual bool updateInputBufferInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual bool updateOutputBufferInfo(Selection& sel) = 0;

    virtual bool updateMultiCamInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual int getSelIndex(Selection& sel) = 0;

    virtual int getSelCount(Selection& sel) = 0;

    virtual MINT64 getTuningIndex(Selection& sel) = 0;

    virtual MINT32 getUniqueKey(Selection& sel) = 0;

    virtual MBOOL getRecommendShot(std::unordered_map<MUINT32, std::shared_ptr<MFNRShotInfo>>& Shots, Selection& sel, MINT64 HidlQueryIndex) = 0;

    virtual MBOOL isNeedDCESO(std::shared_ptr<MFNRShotInfo> pShotInfo) = 0;

    virtual MSize getYUVAlign() = 0;

    virtual MSize getQYUVAlign() = 0;

    virtual MSize getMYUVAlign() = 0;

    virtual void setSrcSize(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual void updateInputMeta(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, IMetadata* pHalMeta, MINT64 key) = 0;

    virtual int updateSelection(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual bool getEnableMfnr(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual MINT64 generateRecommendedKey(MINT32 queryIndex) = 0;

    virtual bool publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, MINT64 key) = 0;

    virtual void publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, MINT64 key) = 0;

};// class IMFNRCapability


class IMFNRCapabilityFactory
{

public:
    static std::shared_ptr<IMFNRCapability> createCapability();

};// class IMFNRCapabilityFactory


} // namespace plugin
} // namespace NSCam
#endif // _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_IMFNRCapability_H_