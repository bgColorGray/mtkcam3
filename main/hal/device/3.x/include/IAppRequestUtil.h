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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPRQUESTUTIL_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPRQUESTUTIL_H_
//
#include <IAppCommonStruct.h>
//
using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {

/**
 * An interface of App stream manager.
 */
class IAppRequestUtil
    : public virtual android::RefBase
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Internal.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
    // FMQ
    // using ResultMetadataQueue = ::android::hardware::MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;
    // using RequestMetadataQueue = ::android::hardware::MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite>;

    // StreamInfo & StreamBuffer
    using AppImageStreamBuffer  = NSCam::v3::Utils::AppImageStreamBuffer;
    // using AppBufferHandleHolder = NSCam::v3::Utils::AppImageStreamBuffer::AppBufferHandleHolder;
    using AppMetaStreamBuffer   = NSCam::v3::Utils::AppMetaStreamBuffer;
    using AppImageStreamInfo    = NSCam::v3::Utils::Camera3ImageStreamInfo;
    using AppMetaStreamInfo     = NSCam::v3::Utils::MetaStreamInfo;

    static auto     create(
                        const CreationInfo& creationInfo/*,
                        std::shared_ptr<RequestMetadataQueue> pRequestMetadataQueue*/
                    ) -> IAppRequestUtil*;

    /**
     * Destroy the instance.
     */
    virtual auto    destroy() -> void                                       = 0;

    /*
     * copy config image/meta map into AppStreamMgr.FrameHandler
     */
    virtual auto    setConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                        )  -> void                                          = 0;

    /**
     * Submit a set of requests.
     * This call is valid only after streams are configured successfully.
     *
     * @param[in] rRequests: a set of requests, created by CameraDeviceSession,
     *  associated with the given CaptureRequest.
     *
     * @return
     *      0 indicates success; otherwise failure.
     */
    virtual auto    createRequests(
                        const std::vector<CaptureRequest>& requests,
                        android::Vector<Request>& rRequests,
                        const ImageConfigMap* imgCfgMap,
                        const MetaConfigMap* metaCfgMap
                    ) -> int                                            = 0;

    /**
     * convert from StreamBuffer to AppImageStreamBuffer
     *
     * @param[in] streamBuffer : stream buffer defined in android type
     * @param[in] pStreamBuffer : stream buffer defined in MTK type
     */
    // virtual auto    convertStreamBuffer(
    //                     const StreamBuffer& streamBuffer,
    //                     android::sp<AppImageStreamBuffer>& pStreamBuffer
    //                 ) const -> int                                      = 0;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_INCLUDE_IAPPREQUESTUTIL_H_

