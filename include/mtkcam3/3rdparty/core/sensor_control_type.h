/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_SENSOR_CONTROL_TYPE_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_SENSOR_CONTROL_TYPE_H_
//
#include <mtkcam/def/common.h>
//
#include <vector>
#include <unordered_map>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace sensorcontrol {

/**
 *  It is used for storing data of sensor.
 */
using SensorId_T = uint32_t;
template<typename Value_T>
using SensorMap = std::unordered_map<uint32_t, Value_T>;

enum SensorStatus
{
    E_NONE,
    E_STANDBY,
    E_STREAMING,
};

enum FeatureMode
{
    E_None,
    E_Zoom,
    E_Bokeh,
    E_Multicam,
};

struct SensorControlParamInit
{
    IMetadata const* logicalStaticMeta = nullptr;
    SensorMap<IMetadata const*> vPhysicalStaticMeta;
};

struct SensorGroupInfo
{
    // Info for exclusive sensor group decision
    int openId = -1;
    int pipDeviceCount = 0;          // 0: non pip, 1: back+back, 2: front+back
    std::vector<int> sensorId;

    // (optional)
    SensorMap<float> physicalFovMap; // FovH

    // result
    // sensors in the same group can't streaming together
    SensorMap<int>* pGroupMap;       // groupId
};

struct SensorControlInfo
{
    SensorStatus iSensorStatus = SensorStatus::E_NONE;
    MBOOL  bAFDone      = false;
    // lastest iso from 3a.
    MINT32 iAEIso       = -1;
    MINT32 iAELV_x10    = -1;
    MINT32 eSensorStatus = -1;
    //
    float fSensorFov_H  = .0f;
    float fSensorFov_V  = .0f;
    MSize mRrzoSize = MSize(0,0);
    MSize msSensorFullSize = MSize(0,0);
    MRect mActiveArrayRegion; // active array size, store in android.sensor.info.activeArraySize.
    MFLOAT mFocalLength = .0f;
};

struct SensorControlResult
{
    SensorStatus iSensorControl = SensorStatus::E_NONE;
    float fAlternavtiveFov_H = .0f;
    float fAlternavtiveFov_V = .0f;
    MRect mrAlternactiveCropRegion;
    MRect mrAlternactiveCropRegion_Cap;
    MRect mrAlternactiveCropRegion_3A;
    std::vector<MINT32> mrAlternactiveAfRegion;
    std::vector<MINT32> mrAlternactiveAeRegion;
    std::vector<MINT32> mrAlternactiveAwbRegion;
    bool isMaster = false;
};

struct SensorControlParamIn
{
    FeatureMode mFeatureMode = FeatureMode::E_None;
    //
    uint32_t mRequestNo = 0;
    //
    int32_t pipDeviceCount = 0;
    //
    IMetadata const* pAppControl = nullptr;
    //
    std::vector<int32_t> vSensorIdList;
    // for physical stream specific sensor
    std::vector<int32_t> physicalSensorIdList;
    // <sensor id, sensor info>
    SensorMap<std::shared_ptr<SensorControlInfo> > vInfo;
    //
    MRect mrCropRegion; // active domain and pass by android.scaler.cropRegion.
    std::vector<MINT32> mrAfRegion;
    std::vector<MINT32> mrAeRegion;
    std::vector<MINT32> mrAwbRegion;
    //
    std::vector<MFLOAT> mAvailableFocalLength;
    //
    MFLOAT mFocalLength;
    //
    MFLOAT mZoomRatio = 0;
    // App iso
    MINT32 mISO;
    //
    MINT32 mEV;
    // Seamless Switch
    bool mbInSensorZoomEnable = false;
    float mZoom2TeleRatio = 0.0f;
};

struct SensorControlParamOut
{
    float fZoomRatio;
    // <sensor id, sensor info>
    bool isZoomControlUpdate = false;
    SensorMap<std::shared_ptr<SensorControlResult> > vResult;
};

};  //namespace sensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_SENSOR_CONTROL_TYPE_H_

