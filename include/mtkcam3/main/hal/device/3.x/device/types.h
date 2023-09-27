/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_MAIN_HAL_DEVICE_3_X_DEVICE_TYPES_H_
#define _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_MAIN_HAL_DEVICE_3_X_DEVICE_TYPES_H_

#include <mtkcam3/main/hal/device/3.x/utils/streaminfo/AppImageStreamInfo.h>
#include <mtkcam3/main/hal/device/3.x/utils/streambuffer/AppStreamBuffers.h>
#include <mtkcam3/pipeline/stream/IStreamBufferProvider.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam3/pipeline/utils/streaminfo/MetaStreamInfo.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metadata/IMetadataConverter.h>
#include <mtkcam/utils/gralloc/IGrallocHelper.h>

#include <utils/RefBase.h>

#include <memory>
#include <unordered_map>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {

using   AppImageStreamInfo = NSCam::v3::Utils::Camera3ImageStreamInfo;
using   AppMetaStreamInfo  = NSCam::v3::Utils::MetaStreamInfo;

typedef android::KeyedVector<StreamId_T, android::sp<AppImageStreamInfo> >       ImageConfigMap;
typedef android::KeyedVector<StreamId_T, android::sp<AppMetaStreamInfo> >        MetaConfigMap;

struct  Request
{
    /*****
     * Assigned by App Stream Manager.
     */

    /**
     * @param frame number.
     */
    uint32_t    requestNo;

    /**
     * @param input image stream buffers.
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<Utils::AppImageStreamBuffer>
        >       vInputImageBuffers;

    /**
     * @param output image stream buffers.
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<Utils::AppImageStreamBuffer>
        >       vOutputImageBuffers;

    /**
     * @param output image stream info.
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<IImageStreamInfo>
        >       vOutputImageStreams;

    /**
     * @param input meta stream buffers.
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<IMetaStreamBuffer>
        >       vInputMetaBuffers;

};

struct  ConfigAppStreams
{
    /**
     * @param operation mode.
     */
    uint32_t    operationMode;

    /**
     * @param image streams.
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<IImageStreamInfo>
        >       vImageStreams;

    /**
     * @param logical meta streams
     */
    android::KeyedVector<
        StreamId_T,
        android::sp<IMetaStreamInfo>
        >       vMetaStreams;

    /**
     * @param physical meta streams
     */
    android::KeyedVector<
        MINT32,
        android::sp<IMetaStreamInfo>
        >       vMetaStreams_physical;

    /**
     * @param physic id list.
     */
    // std::vector<MINT32>  vPhysicCameras;
};

struct  PhysicalResult
    : public android::RefBase
{
    /**
     * @param[in] physic camera id
     */
     std::string physicalCameraId;

    /**
     * @param[in] physic result partial metadata to update.
     */
     android::sp<IMetaStreamBuffer> physicalResultMeta;
};

/**
 * This tag in UpdateResultParams decides the partial metadata in request
 * should be callbacked in which mode.
 * This might only effect while MetaPending is enable.
 */
enum class RealTime : uint32_t {
    /**
     * If NON, the result callback would be pending util the next real-time result
     * (or the last result arrives) in the same request arrives.
     */
    NON = 0,
    /**
     * If SOFT, the result would be callbacked with the next last partial (maybe previous request).
     */
    SOFT = 1,
    /**
     * If HARD, the result would be callbacked instantly.
     */
    HARD = 2,
};

struct UpdateResultParams
{
    /**
     * @param[in] the frame number to update.
     */
    uint32_t                                    requestNo = 0;

    /**
     * @param[in] user id.
     *  In fact, it is pipeline node id from the viewpoint of pipeline implementation,
     *  but the pipeline users have no such knowledge.
     */
    intptr_t                                    userId = 0;

    /**
     * @param[in] hasLastPartial: contain last partial metadata in result partial metadata vector.
     */
    bool                                        hasLastPartial = false;

    /**
     * @param[in] timestampStartOfFrame: referenced by De-jitter mechanism.
     */
    int64_t                                     timestampStartOfFrame = 0;

    /**
     * @param[in] realTimePartial: contain real-time partial metadata in result partial metadata vector.
     */
    RealTime                                    realTimePartial = RealTime::NON;

    /**
     * @param[in] result partial metadata to update.
     */
    android::Vector<android::sp<IMetaStreamBuffer>> resultMeta;

    /**
     * @param[in] phsycial result to update.
     */
    android::Vector<android::sp<PhysicalResult>> physicalResult;

};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_MAIN_HAL_DEVICE_3_X_DEVICE_TYPES_H_

