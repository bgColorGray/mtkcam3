/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_TYPES_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_TYPES_H_

//#include <mtkcam-android/policy/Types.h>
//#include <mtkcam-android/pipeline/model/Types.h>

#include <mtkcam3/pipeline/policy/types.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam3/feature/eis/EisInfo.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/def/ImageBufferInfo.h>
#include <mtkcam/def/SensorMap.h>
#include <mtkcam/def/common.h>
#include <mtkcam/def/CameraInfo.h>

#include <set>
#include <vector>
#include <memory>
#include <unordered_map>

//using NSCam::v3::pipeline::policy::ParsedMultiCamInfo;
using NSCam::v3::pipeline::policy::ParsedSMVRBatchInfo;
using NSCam::v3::IImageStreamInfo;
using NSCam::v3::IMetaStreamInfo;
using NSCam::CameraInfoMapT;
//using NSCam::SensorMap;
using NSCam::v3::StreamId_T;
using NSCam::IMetadata;
//using NSCam::SecType;
//using NSCam::MSize;
//using NSCam::SecureInfo;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace device {
namespace policy {

enum class PipelineSessionType : uint32_t {
    BASIC = 0,
    SMVR,
    SMVR_BATCH,
    MULTICAM,
    MULTICAM_VSDOF,
    MULTICAM_ZOOM,
    MULTI_CELL,  // 4cell, 9cell,...,etc
};

/**
 *  Policy static information
 */
struct PolicyStaticInfo {
    /**
     * uniqueKey for this CaptureSession
     */
    uint32_t              uniqueKey = 0;
    /**
     * Logical device open id
     */
    int32_t               openId = -1;
    /**
     * Physical sensor id (0, 1, 2)
     */
    std::vector<int32_t>  sensorId;
    /**
     * secure device
     */
    bool                  isSecureDevice = false;
    /**
     * Has more than one P1
     */
    bool                  isDualDevice = false;
    /**
     *  is 4-Cell sensor
     */
    bool                  is4CellSensor = false;
    /**
     *  is 4-Cell sensor for all sensors
     */
    std::set<uint32_t>    v4CellSensorIds;
    /**
     * P1 direct output rectify YUV (ISP6.0)
     */
    int32_t               numsP1YUV = -1;
    /**
     *  is P1 direct output FD YUV (ISP6.0)
     */
    bool                  isP1DirectFDYUV = false;
    /**
     * static camera info
     */
    CameraInfoMapT        cameraInfoMap;
    /**
     * session Type (SMVR, VSDOF, 4Cell,...,etc)
     */
    mcam::core::device::policy::PipelineSessionType sessionType;
};

//struct PolicyParsedAppConfiguration;
//struct PolicyParsedAppImageStreamInfo;
//
///**
// * Policy user configuration
// *
// * The following information is given and set up at the configuration stage,
// * and is never changed AFTER the configuration stage.
// */
//struct PolicyUserConfiguration {
//    std::shared_ptr<PolicyParsedAppConfiguration> pParsedAppConfiguration;
//
//    /**
//     * Parsed App image stream info set
//     *
//     * It results from the raw data, i.e. vImageStreams.
//     */
//    std::shared_ptr<PolicyParsedAppImageStreamInfo> pParsedAppImageStreamInfo;
//};
//
///**
// * Parsed App configuration
// */
//struct PolicyParsedAppConfiguration {
//    /**
//     * The operation mode of pipeline.
//     * The caller must promise its value.
//     */
//    uint32_t operationMode = 0;
//
//    /**
//     * Session wide camera parameters.
//     *
//     * The session parameters contain the initial values of any request keys that
//     * were made available via ANDROID_REQUEST_AVAILABLE_SESSION_KEYS. The Hal
//     * implementation can advertise any settings that can potentially introduce
//     * unexpected delays when their value changes during active process requests.
//     * Typical examples are parameters that trigger time-consuming HW
//     * re-configurations or internal camera pipeline updates. The field is
//     * optional, clients can choose to ignore it and avoid including any initial
//     * settings. If parameters are present, then hal must examine their values and
//     * configure the internal camera pipeline accordingly.
//     */
//    IMetadata sessionParams;
//
//    /**
//     * operationMode = 1
//     *
//     * StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE = 1
//     * Refer to
//     * https://developer.android.com/reference/android/hardware/camera2/params/SessionConfiguration#SESSION_HIGH_SPEED
//     */
//    bool isConstrainedHighSpeedMode = false;
//
//    /**
//     * Dual cam related info
//     */
//    std::shared_ptr<ParsedMultiCamInfo> pParsedMultiCamInfo;
//
//    /**
//     * SMVRBatch related info.
//     */
//    std::shared_ptr<ParsedSMVRBatchInfo> pParsedSMVRBatchInfo;
//
//    bool supportAIShutter = false;
//
//    bool hasTuningEnable = false;
//};
//
///**
// * Parsed App image stream info
// */
//struct PolicyParsedAppImageStreamInfo {
//    /**************************************************************************
//     *  App image stream info set
//     **************************************************************************/
//    /**
//     * Output streams for hal3+ usage
//     *
//     * Reference:
//     * Hal3+ document
//     */
//    SensorMap<std::vector<std::shared_ptr<IImageStreamInfo>>>
//        vCustomImage_Output;
//
//    /**
//     * Output streams for any processed (but not-stalling) formats
//     *
//     * Reference:
//     * https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics.html#REQUEST_MAX_NUM_OUTPUT_PROC
//     */
//    std::unordered_map<StreamId_T, std::shared_ptr<IImageStreamInfo>>
//        vAppImage_Output_Proc;
//
//    /**
//    * Output stream for private reprocessing
//    */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Output_Priv;
//
//    /**
//     * Input stream for yuv reprocessing
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Input_Yuv;
//
//    /**
//     * Input stream for private reprocessing
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Input_Priv;
//
//    /**
//     * Output stream for RAW16/DNG capture.
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Output_RAW16;
//
//    /**
//     * Input stream for RAW16 reprocessing.
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Input_RAW16;
//
//    /**
//     * Output stream for JPEG capture.
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Jpeg;
//
//    /**
//     * Output stream for Depthmap.
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Depth;
//
//    /**
//     * Output stream for physical yuv stream
//     * map : <sensorId, sp<streamInfo>>
//     */
//    SensorMap<std::vector<std::shared_ptr<IImageStreamInfo>>>
//        vAppImage_Output_Proc_Physical;
//
//    /**
//    * Output stream for physical RAW16/DNG capture.
//    * map : <sensorId, sp<streamInfo>>
//    */
//    SensorMap<std::vector<std::shared_ptr<IImageStreamInfo>>>
//        vAppImage_Output_RAW16_Physical;
//
//    /**
//     * Output stream for isp tuning data stream (RAW)
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Output_IspTuningData_Raw;
//
//    /**
//     * Output stream for isp tuning data stream (YUV)
//     */
//    std::shared_ptr<IImageStreamInfo> pAppImage_Output_IspTuningData_Yuv;
//
//    /**
//     * Output stream for physical isp meta stream (Raw)
//     * map : <sensorId, sp<streamInfo>>
//     */
//    SensorMap<std::shared_ptr<IImageStreamInfo>>
//        vAppImage_Output_IspTuningData_Raw_Physical;
//
//    /**
//     * Output stream for physical isp meta stream (Yuv)
//     * map : <sensorId, sp<streamInfo>>
//     */
//    SensorMap<std::shared_ptr<IImageStreamInfo>>
//        vAppImage_Output_IspTuningData_Yuv_Physical;
//
//    /**
//     * One of consumer usages of App image streams contains
//     * BufferUsage::VIDEO_ENCODER.
//     */
//    bool hasVideoConsumer = false;
//
//    /**
//     * The image size of video recording, in pixels.
//     */
//    MSize videoImageSize;
//
//    /**
//     * 4K video recording
//     */
//    bool hasVideo4K = false;
//
//    /**
//     * The image size of app yuv out, in pixels.
//     */
//    MSize maxYuvSize;
//
//    /**
//     * The image size of app stream out, in pixels.
//     */
//    MSize maxStreamSize;
//
//    /**
//     * Security Info
//     */
//    SecureInfo secureInfo{.type = SecType::mem_normal};
//
//    bool hasRawOut = false;
//
//    /**
//     *
//     */
//    StreamId_T previewStreamId = -1;
//};
//
struct AppUserConfiguration {
  /**
   * @param[in] The operation mode of pipeline.
   *  The caller must promise its value.
   */
  uint32_t operationMode = 0;

  /**
   * Session wide camera parameters.
   *
   * The session parameters contain the initial values of any request keys that
   * were made available via ANDROID_REQUEST_AVAILABLE_SESSION_KEYS. The Hal
   * implementation can advertise any settings that can potentially introduce
   * unexpected delays when their value changes during active process requests.
   * Typical examples are parameters that trigger time-consuming HW
   * re-configurations or internal camera pipeline updates. The field is
   * optional, clients can choose to ignore it and avoid including any initial
   * settings. If parameters are present, then hal must examine their values and
   * configure the internal camera pipeline accordingly.
   */
  IMetadata sessionParams;

  /**
   * @param[in] App image streams to configure.
   *  The caller must promise the number of buffers and each content.
   */
  std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>
      vImageStreams;

  /**
   * @param[in] App meta streams to configure.
   *  The caller must promise the number of buffers and each content.
   */
  std::unordered_map<StreamId_T, android::sp<IMetaStreamInfo>> vMetaStreams;

  /**
   * @param[in] App physical meta streams to configure.
   *  The caller must promise the number of buffers and each content.
   *
   * @tparam int32_t sensor ID.
   */
  std::unordered_map<int32_t, android::sp<IMetaStreamInfo>>
      vMetaStreams_physical;

  /**
   * @param[in] App image streams min frame duration to configure.
   *  The caller must promise its initial value.
   */
  std::unordered_map<StreamId_T, int64_t> vMinFrameDuration;

  /**
   * @param[in] App image streams stall frame duration to configure.
   *  The caller must promise its initial value.
   */
  std::unordered_map<StreamId_T, int64_t> vStallFrameDuration;

  /**
   * @param[in] physical camera id list
   */
  std::vector<int32_t> vPhysicCameras;

  /**
   * @param[in] The factory of App stream info builder.
   *
   *  It is used to create IStreamInfoBuilder instances.
   */
  std::shared_ptr<IStreamInfoBuilderFactory>  pStreamInfoBuilderFactory;

  /**
   * please refer to pipeline/model/Types.h
   */
  std::unordered_map<StreamId_T, uint8_t> streamImageProcessingSettings;
};
//
/**
 * Parsed pipeline user configuration
 */
struct PipelineUserConfiguration {
//    using pplStreamConfiguration
//        = NSCam::v3::pipeline::model::StreamConfiguration;
//    using SessionConfiguration
//        = NSCam::v3::pipeline::model::SessionConfiguration;
//    using P1Configuration =
//        NSCam::v3::pipeline::model::P1Configuration;
    std::shared_ptr<AppUserConfiguration>   pAppUserConfiguration;
//    std::shared_ptr<pplStreamConfiguration> pStreamConfiguration;
//    std::shared_ptr<SessionConfiguration>   pSessionConfiguration;
//    std::shared_ptr<P1Configuration>        pP1Configuration;
//    std::shared_ptr<ParsedSMVRBatchInfo>    pParsedSMVRBatchInfo;
};
//
///**
// * Parsed metadata control request
// */
//using PolicyParsedMetaControl = NSCam::v3::pipeline::policy::ParsedMetaControl;
//
//
///**
// * Additional raw info for ISP device.
// */
//struct AdditionalRawInfo {
//  /**
//   *  New StreamInfo for Raw
//   */
//  std::shared_ptr<IImageStreamInfo> streamInfo;
//};
//
//
///**
// * store pipeline sensor state change result.
// */
//struct SensorStateChangedResult {
//};
//
///*
// * Store notify param for policy callback
// */
//struct SensorControlNotify {
//  std::shared_ptr<int32_t> nextCapture = nullptr;
//  std::shared_ptr<int32_t> masterId = nullptr;
//  std::shared_ptr<uint8_t> afState = nullptr;
//};
//struct NotifyParams {
//  uint32_t requestNo;
//  bool hasLastPartial = false;
//  std::shared_ptr<SensorControlNotify> pSensorControlNotify;
//};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace policy
};  // namespace device
};  // namespace core
};  // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_POLICY_TYPES_H_
