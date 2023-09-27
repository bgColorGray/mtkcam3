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

#ifndef _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_IDEVICESESSIONPOLICU_H_
#define _MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_IDEVICESESSIONPOLICU_H_

#include <mtkcam3/main/hal/device/3.x/device/mtkcamhal_types.h>
#include <mtkcam3/main/hal/device/3.x/device/types.h>
#include <mtkcam3/main/hal/device/3.x/utils/streaminfo/AppImageStreamInfo.h>
#include <mtkcam3/main/hal/device/3.x/utils/streambuffer/AppStreamBuffers.h>

#include <mtkcam3/pipeline/model/IPipelineModelManager.h>
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
namespace device {
namespace policy {

using   AppImageStreamInfo = NSCam::v3::Utils::Camera3ImageStreamInfo;
using   AppMetaStreamInfo  = NSCam::v3::Utils::MetaStreamInfo;

typedef std::map<StreamId_T, android::sp<AppImageStreamInfo> >   ImageConfigurationMap;
typedef std::map<StreamId_T, android::sp<AppMetaStreamInfo> >    MetaConfigurationMap;


// static info, immutable when bring-up.
struct DeviceSessionStaticInfo {
    int32_t                                                 mInstanceId = -1;
    std::weak_ptr<CameraDevice3SessionCallback>             mCameraDeviceCallback;
    android::sp<IMetadataProvider>                          mMetadataProvider = nullptr;
    std::map<uint32_t, android::sp<IMetadataProvider>>      mPhysicalMetadataProviders;
    android::sp<IMetadataConverter>                         mMetadataConverter = nullptr;
    std::shared_ptr<android::Printer>                       mErrorPrinter;
    std::shared_ptr<android::Printer>                       mWarningPrinter;
    std::shared_ptr<android::Printer>                       mDebugPrinter;
    // IGrallocHelper*                                         mGrallocHelper = nullptr;
};

// config info, changable only in configuration/reconfiguration stage.
struct DeviceSessionConfigInfo {
    ImageConfigurationMap                                   mImageConfigMap;
    MetaConfigurationMap                                    mMetaConfigMap;
    bool                                                    isSecureCameraDevice = false;
    bool                                                    mSupportBufferManagement = false;
};

struct ConfigurationInputParams
{
    // requested stream from user configuration
    StreamConfiguration const*                              requestedConfiguration;
    // ImageConfigurationMap const*                            currentImageConfigMap;
};

struct ConfigurationSessionParams {
    // parsing from session params
    bool                                                    zslMode = false;
};

struct MetaStreamCreationParams
{

};

// will be moved to feature decision
struct ImageCreationParams
{
    NSCam::ImageBufferInfo                                  imgBufferInfo;
    mcam::GrallocStaticInfo                                 grallocStaticInfo;
    mcam::GrallocRequest                                    grallocRequst;
    bool                                                    isSecureFlow = false;
};

struct ConfigurationOutputParams
{
    // output stream for user configuration
    HalStreamConfiguration*                                 resultConfiguration;
    // std::map<int32_t, unique_ptr<ImageCreationParams> >     imageCreationParams;
    // pipeline model configuration input params (UserConfigurationParams)
    std::shared_ptr<pipeline::model::UserConfigurationParams>* pipelineCfgParams;
    ImageConfigMap*                                         imageConfigMap;
    MetaConfigMap*                                          metaConfigMap;
    // IMetadata                                               sessionParams;
    // std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>
    //                                                         vImageStreams;
    // std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>>
    //                                                         vMetaStreams;
    // std::unordered_map<int32_t, android::sp<IMetaStreamInfo>>
    //                                                         vMetaStreams_physical;
    // std::unordered_map<StreamId_T, int64_t>                 vMinFrameDuration;
    // std::unordered_map<StreamId_T, int64_t>                 vStallFrameDuration;
    // std::vector<int32_t>                                    vPhysicCameras;
    // std::shared_ptr<IStreamInfoBuilderFactory>              pStreamInfoBuilderFactory;
    // std::shared_ptr<IImageStreamBufferProvider>             pImageStreamBufferProvider;
    // uint64_t                                                configTimestamp = 0;
};

struct RequestInputParams
{
    std::vector<CaptureRequest> const*                      requests;
};

struct RequestOutputParams
{
    android::Vector<Request>*                               requests;
};

struct ResultInputParams
{

};

struct ResultOutputParams
{

};

struct ResultParams
{
    UpdateResultParams*                                     params;
};

class IDeviceSessionPolicy
{
public:
    virtual ~IDeviceSessionPolicy() = default;

    virtual auto    evaluateConfiguration(
                        ConfigurationInputParams const* in,
                        ConfigurationOutputParams* out
                    ) -> int                                                        = 0;

    virtual auto    evaluateRequest(
                        RequestInputParams const* in,
                        RequestOutputParams* out
                    ) -> int                                                        = 0;

    virtual auto    processResult(
                        // ResultInputParams const* in,
                        // ResultInOutputParams* out
                        ResultParams* params
                    ) -> int                                                        = 0;

    virtual auto    destroy() -> void                                               = 0;
};


class IDeviceSessionPolicyFactory
{
public:     ////
    /**
     * A structure for creation parameters.
     */
    struct CreationParams
    {
        std::shared_ptr<DeviceSessionStaticInfo const>      mStaticInfo;
        std::shared_ptr<DeviceSessionConfigInfo const>      mConfigInfo;
    };

    static auto     createDeviceSessionPolicyInstance(
                        CreationParams const& params
                    ) -> std::shared_ptr<IDeviceSessionPolicy>;
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace policy
};  //namespace device
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM3_MAIN_HAL_ENTRY_DEVICE_INCLUDE_IDEVICESESSIONPOLICU_H_

