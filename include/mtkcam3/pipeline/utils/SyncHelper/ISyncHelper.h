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

#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_V3_SYNCHELPER_ISYNCHELPER_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_V3_SYNCHELPER_ISYNCHELPER_H_

#include <utils/Errors.h>

#include <mtkcam/utils/metadata/IMetadata.h>
#include <unordered_map>

using namespace android;

/******************************************************************************
 *ISyncHelper is used to convert IMetadata for ISyncHelpBase
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {
namespace Imp {
enum SyncType
{
    NONE,
    ENQ_HW,
    HW_RESULT_CHECK,
};


/******************************************************************************
 *
 ******************************************************************************/
struct SyncResultInputParams
{
    int CamId = -1;
    int requestNum = -1;
    int frameNum = -1;
    IMetadata* HalControl = nullptr;
};

/******************************************************************************
 *
 ******************************************************************************/
struct SyncResultOutputParams
{
    IMetadata* HalDynamic = nullptr;
    IMetadata* AppDynamic = nullptr;
    bool bMasterCam = true;
};

/******************************************************************************
 *
 ******************************************************************************/
class ISyncHelper : public virtual android::RefBase
{
public:
    virtual ~ISyncHelper() = default;
public:
    virtual status_t start(int CamId) = 0;
    virtual status_t stop(int CamId) = 0;
    virtual status_t init(int CamId) = 0;
    virtual status_t uninit(int CamId) = 0;
    virtual status_t flush(int CamId) = 0;

    virtual status_t syncEnqHW(
                                SyncResultInputParams const& input) = 0;
    virtual status_t notifyDrop(
                                SyncResultInputParams const& input) = 0;
    /*Return the Sync Result*/
    virtual bool syncResultCheck(
                                SyncResultInputParams const& input,
                                SyncResultOutputParams &output) = 0;

    /*Check has user in synchelper*/
    virtual bool hasOtherUser(int CamId) = 0;

    virtual bool setSensorCropRegion(int requestNum, std::unordered_map<int32_t, MRect>& cropList) = 0;

    virtual bool getPreQueue(std::vector<int>& frameNoList) = 0;
    virtual int setPreQueue(int frameNo) = 0;
    static android::sp<ISyncHelper> createInstance(int32_t openId, uint64_t timestamp, int32_t initRequest, bool enableCropOverride);
};
/******************************************************************************
 *
 ******************************************************************************/
}
}
}
}
/******************************************************************************
 *
 ******************************************************************************/

#endif // _MTK_HARDWARE_INCLUDE_MTKCAM_V3_HWNODE_SYNCHELPER_H_
