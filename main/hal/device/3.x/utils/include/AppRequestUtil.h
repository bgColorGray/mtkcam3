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

#ifndef _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_
#define _MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_
//
#include <IAppRequestUtil.h>
//
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/Timers.h>
#include <utils/List.h>
#include <utils/Condition.h>
//
#include <time.h>
#include <vector>
#include <memory>

using namespace NSCam::v3::Utils;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace Utils {

class AppRequestUtil
    : public IAppRequestUtil
{
    friend  class IAppRequestUtil;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definition.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:  ////
    // CommonInfo
    struct  CommonInfo
    {
        int32_t                                     mLogLevel = 0;
        int32_t                                     mInstanceId = -1;
        std::shared_ptr<android::Printer>           mErrorPrinter = nullptr;
        std::shared_ptr<android::Printer>           mWarningPrinter = nullptr;
        std::shared_ptr<android::Printer>           mDebugPrinter = nullptr;
        std::weak_ptr<CameraDevice3SessionCallback> mDeviceCallback;
        android::sp<IMetadataProvider>              mMetadataProvider = nullptr;
        std::map<uint32_t, android::sp<IMetadataProvider>> mPhysicalMetadataProviders;
        android::sp<IMetadataConverter>             mMetadataConverter = nullptr;
        IGrallocHelper*                             mGrallocHelper = nullptr;
        size_t                                      mAtMostMetaStreamCount = 0;
        bool                                        mSupportBufferManagement = false;
    };


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    std::string const           mInstanceName;
    std::shared_ptr<CommonInfo> mCommonInfo = nullptr;

    mutable android::Mutex       mImageConfigMapLock;
    ImageConfigMap mImageConfigMap;
    MetaConfigMap  mMetaConfigMap;

    // std::shared_ptr<RequestMetadataQueue>
    //                             mRequestMetadataQueue = nullptr;
    IMetadata                   mLatestSettings;
    // ion device for allocate graphic buffer
    std::shared_ptr<IIonDevice>  mIonDevice = nullptr;

protected:  ////

    auto            getLogLevel() const -> int32_t { return mCommonInfo != nullptr ? mCommonInfo->mLogLevel : 0; }

    auto            checkOneRequest(
                        const CaptureRequest& request
                    )   const -> int;

    auto            createOneRequest(
                        const CaptureRequest& request,
                        Request& rRequest,
                        const ImageConfigMap* imgCfgMap,
                        const MetaConfigMap* metaCfgMap
                    )   -> int;

    // auto            convertStreamBuffer(
    //                     const std::string& bufferName,
    //                     const StreamBuffer& streamBuffer,
    //                     android::sp<AppImageStreamBuffer>& pStreamBuffer/*,
    //                     const BufferHandle& rBufferHandle*/
    //                 )   const -> int;

    // auto            importStreamBuffer(
    //                     const StreamBuffer& streamBuffer,
    //                     // buffer_handle_t& bufferHandle,
    //                     // std::shared_ptr<AppBufferHandleHolder>& appBufferHandleHolder,
    //                     int& acquire_fence
    //                 )   const -> int;

    // auto            createImageStreamBuffer(
    //                     const std::string& bufferName,
    //                     const StreamBuffer& streamBuffer,
    //                     buffer_handle_t bufferHandle,
    //                     // std::shared_ptr<AppBufferHandleHolder> appBufferHandleHolder,
    //                     int const acquire_fence,
    //                     int const release_fence
    //                 )   const -> AppImageStreamBuffer*;

    auto            createMetaStreamBuffer(
                        android::sp<IMetaStreamInfo> pStreamInfo,
                        IMetadata const& rSettings,
                        bool const repeating
                    )   const -> AppMetaStreamBuffer*;

    auto            getConfigMetaStream(
                        StreamId_T streamId,
                        const MetaConfigMap* metaCfgMap
                    )   const -> android::sp<AppMetaStreamInfo>;

    auto            getConfigImageStream(
                        StreamId_T streamId,
                        const ImageConfigMap* imgCfgMap
                    )   const -> android::sp<AppImageStreamInfo>;

    // auto            importBuffer(
    //                     StreamId_T streamId,
    //                     uint64_t bufferId,
    //                     buffer_handle_t& importedBufferHandle,
    //                     std::shared_ptr<AppBufferHandleHolder>& appBufferHandleHolder
    //                 ) const -> int;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Operations.
                    AppRequestUtil(
                        const CreationInfo& creationInfo/*,
                        std::shared_ptr<RequestMetadataQueue> pRequestMetadataQueue*/
                    );

    virtual auto    destroy() -> void;

    virtual auto    setConfigMap(
                        ImageConfigMap& imageConfigMap,
                        MetaConfigMap& metaConfigMap
                    )  -> void;

    virtual auto    reset() -> void;

    virtual auto    createRequests(
                        const std::vector<CaptureRequest>& requests,
                        android::Vector<Request>& rRequests,
                        const ImageConfigMap* imgCfgMap,
                        const MetaConfigMap* metaCfgMap
                    ) -> int;

    // virtual auto    convertStreamBuffer(
    //                     const StreamBuffer& streamBuffer,
    //                     android::sp<AppImageStreamBuffer>& pStreamBuffer
    //                 ) const -> int;

};

/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_MAIN_HAL_DEVICE_3_X_APP_APPSTREAMMGR_H_