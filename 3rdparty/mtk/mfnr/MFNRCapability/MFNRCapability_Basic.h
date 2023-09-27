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
#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_MFNRCapability_Basic_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_MFNRCapability_Basic_H_

#include <stdlib.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>

#include <mtkcam/utils/hw/FleetingQueue.h>

#include "MFNRShotInfo.h"
#include "../inc/IMFNRCapability.h"

using namespace android;
using namespace mfll;

namespace NSCam {
namespace plugin {

#define ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT 1

#ifdef ISP_HIDL_TEMP_SOL_WITHOUT_TUNING_INDEX_HINT
// HIDL ISP shot instance
struct RecommendShooter {
    std::shared_ptr<MFNRShotInfo> shotInstance;
    MINT32 sensorId = -1;
    RecommendShooter () : shotInstance(nullptr) { };
    ~RecommendShooter () { shotInstance = nullptr; };
};
static MUINT32 mRecommendCount;
static std::mutex mRecommendCountMx; // protect mRecommendCount
//
static NSCam::FleetingQueue<RecommendShooter, 30, 2> mRecommendQueue;
#else
// do nothing
#endif


/******************************************************************************
*
******************************************************************************/
class MFNRCapability_Basic : public IMFNRCapability
{
    typedef NSPipelinePlugin::MultiFramePlugin::Selection Selection;

// Constructor & destructor
//
public:
    MFNRCapability_Basic();
    virtual ~MFNRCapability_Basic();

public:
    virtual void setShotInfo(std::shared_ptr<MFNRShotInfo> pShotinfo) { mpShotInfo = pShotinfo; };

    virtual bool updateInputBufferInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual bool updateOutputBufferInfo(Selection& sel) = 0;

    virtual bool updateMultiCamInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel) = 0;

    virtual int getSelIndex(Selection& sel);

    virtual int getSelCount(Selection& sel);

    virtual MINT64 getTuningIndex(Selection& sel);

    virtual MINT32 getUniqueKey(Selection& sel);

    virtual MBOOL getRecommendShot(std::unordered_map<MUINT32, std::shared_ptr<MFNRShotInfo>>& Shots, Selection& sel, MINT64 HidlQueryIndex);

    virtual MBOOL isNeedDCESO(std::shared_ptr<MFNRShotInfo> /*pShotInfo*/) { return MFALSE; };

    virtual MSize getYUVAlign() = 0;

    virtual MSize getQYUVAlign() = 0;

    virtual MSize getMYUVAlign() { return MSize(16, 16); };

    virtual void setSrcSize(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel);

    virtual void updateInputMeta(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, IMetadata* pHalMeta, MINT64 key);

    virtual int updateSelection(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel);

    virtual bool getEnableMfnr(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel);

    virtual MINT64 generateRecommendedKey(MINT32 queryIndex);

    virtual bool publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, Selection& sel, MINT64 key);

    virtual void publishShotInfo(std::shared_ptr<MFNRShotInfo> pShotInfo, MINT64 key);

protected:
    virtual MSize calcDownScaleSize(const MSize& m, int dividend, int divisor);

    virtual MINT32 getQueryIndexFromRecommendedKey(MINT64 recommendKey);

private:
    std::shared_ptr<MFNRShotInfo> mpShotInfo;
    int                           m_dbgLevel;

};// class MFNRCapability_Basic
} // namespace plugin
} // namespace NSCam
#endif // _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_PLUGIN_MFNR_MFNRCapability_Basic_H_