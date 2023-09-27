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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPCONFIGUTIL_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPCONFIGUTIL_H_

#include "MyUtils.h"
//
#include <utils/Mutex.h>
#include <utils/BitSet.h>
#include <utils/KeyedVector.h>
//
#include <IAppCommonStruct.h>
//
#include <mtkcam3/pipeline/model/IPipelineModelManager.h>

using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {

/**
 * An interface of App configuration utility.
 */
class IAppConfigUtil
    : public virtual android::RefBase
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
    /**
     * Create an instance.
     */
    static auto     create(const CreationInfo& creationInfo) -> IAppConfigUtil*;

    /**
     * Destroy the instance.
     */
    virtual auto    destroy() -> void = 0;

    /**
     * Create stream info.
     */
    virtual auto    beginConfigureStreams(
                        const StreamConfiguration& requestedConfiguration,
                        HalStreamConfiguration& halConfiguration,
                        std::shared_ptr<pipeline::model::UserConfigurationParams>& rCfgParams
                    )   -> ::android::status_t = 0;

    // /**
    //  * get image stream info.
    //  */
    // virtual auto    getConfigImageStream(
    //                     StreamId_T streamId
    //                 ) const -> android::sp<AppImageStreamInfo> = 0;

    // /**
    //  * get meta stream info.
    //  */
    // virtual auto    getConfigMetaStream(
    //                     StreamId_T streamId
    //                 ) const -> android::sp<AppMetaStreamInfo> = 0;

    /**
     * get app stream info builder factory.
     */
    virtual auto    getAppStreamInfoBuilderFactory(
                    ) const -> std::shared_ptr<IStreamInfoBuilderFactory>  = 0;

    /**
     * set Config Map
     */
    virtual auto    getConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                    ) -> void = 0;

};

};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPCONFIGUTIL_H_

