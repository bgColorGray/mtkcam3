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

#define LOG_TAG "mtkcam-customer_sensor_control"
//
#include <cutils/properties.h>
//
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/common.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
//
#include <mtkcam3/3rdparty/customer/customer_sensor_control.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>
//
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <sstream>

/******************************************************************************
 *
 ******************************************************************************/
#define __DEBUG // enable function scope debug
#ifdef __DEBUG
#include <memory>

#define FUNCTION_SCOPE \
 auto __scope_logger__ = [](char const* f)->std::shared_ptr<const char>{ \
    CAM_ULOGMD("(%d)[%s] + ", ::gettid(), f); \
    return std::shared_ptr<const char>(f, [](char const* p){CAM_ULOGMD("(%d)[%s] -", ::gettid(), p);}); \
}(__FUNCTION__)
#else
#define FUNCTION_SCOPE
#endif

CAM_ULOG_DECLARE_MODULE_ID(MOD_PIPELINE_POLICY);
/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
/******************************************************************************
 *
 ******************************************************************************/

#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
/******************************************************************************
 *
 ******************************************************************************/

namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace sensorcontrol {

bool customer_decision_sensor_control(
    SensorControlParamOut &out  __attribute__((unused)),
    SensorControlParamIn const& in  __attribute__((unused))
)
{
    // if customer implement this function, it has to change return value to ture.
    return false;
}

auto
CustomerSensorControl::
createInstance(
    uint64_t timestamp __attribute__((unused))
) -> std::shared_ptr<CustomerSensorControl>
{
    //return std::make_shared<CustomerSensorControl>(timestamp);
    return nullptr;
}

CustomerSensorControl::
CustomerSensorControl(
    uint64_t timestamp
)
{
    mTimestamp = timestamp;
}

auto
CustomerSensorControl::
init(SensorControlParamInit param __attribute__((unused))) -> bool
{
    mLogLevel = ::property_get_int32("vendor.debug.policy.sensorcontrol", -1);
    // create 3rd reader
    mp3rdReader = NSCam::IGenericContainer<std::shared_ptr<Customer_FOVA_Streaming_Data>>::makeReader(
                                                        LOG_TAG,
                                                        mTimestamp,
                                                        (uint32_t)IGenericContainerKey::VENDOR_FOVA);
    mPreviewSize = StereoSizeProvider::getInstance()->getPreviewSize();
    MY_LOGD_IF(mLogLevel >= 1,"previewSize:%dx%d",mPreviewSize.w, mPreviewSize.h);

    return false;
}

auto
CustomerSensorControl::
decideSensorGroup(SensorGroupInfo& info) -> bool
{
    // must set exclusive group here
    // Query default group from custom setting file
    for (auto const& item : StereoSettingProvider::getExclusiveGroup(info.openId)) {
        (*info.pGroupMap).emplace(item.first, item.second);
    }

    // customize here

    return true;
}

bool CustomerSensorControl::setCustomZoneSensorStatus(
        SensorControlParamOut& out,
        SensorControlParamIn const& in) {
  auto sensorCount = in.vSensorIdList.size();
  auto updateSensorState = [&out](int32_t sensorId,
                                  bool isMaster,
                                  SensorStatus status) {
    auto iter = out.vResult.find(sensorId);
    if (iter != out.vResult.end() && iter->second != nullptr) {
      iter->second->isMaster = isMaster;
      iter->second->iSensorControl = status;
      return true;
    } else {
      return false;
      }
  };

  // parse master
  int32_t previewMaster =
            property_get_int32("vendor.debug.camera.custom.mastercam", -1);
  if (previewMaster != -1) {
    MY_LOGD("debug: preview master(%d)", previewMaster);
  } else {
    auto entry =
      in.pAppControl->entryFor(MTK_HALCORE_PHYSICAL_MASTER_ID);
    if (entry.count() == 1) {
      previewMaster = entry.itemAt(0, Type2Type<MINT32>());
      MY_LOGD("custom zone: previewMaster(%d)", previewMaster);
    } else {
      previewMaster = in.vSensorIdList[0];
      MY_LOGE("custom zone: invalid master, use sensor(%d)", previewMaster);
    }
  }

  // parse sensor status
  char tempbuf[1024]="";
  property_get("vendor.debug.camera.custom.forceStreamingIds", tempbuf, "-1");
  bool forceSensorStreaming = (0 != strcmp(tempbuf, "-1"));
  if (forceSensorStreaming) {
    std::istringstream iss((std::string(tempbuf)));
    std::set<int> parsedSensors;
    for (std::string token; std::getline(iss, token, ','); ) {
      parsedSensors.emplace(std::stoi(token));
    }
    for (auto const& sensorId : in.vSensorIdList) {
      bool isMaster = (sensorId == previewMaster) ? true : false;
      if (parsedSensors.count(sensorId) != 0) {
        MY_LOGD("debug: force streaming sensor(%d)", sensorId);
        updateSensorState(sensorId, isMaster, SensorStatus::E_STREAMING);
      } else {
        updateSensorState(sensorId, isMaster, SensorStatus::E_STANDBY);
      }
    }
    return true;
  }
  else {
    auto entry =
      in.pAppControl->entryFor(MTK_HALCORE_PHYSICAL_SENSOR_STATUS);
    // all sensors and its status must be filled.
    if (!entry.isEmpty() && (entry.count() == sensorCount * 2)) {
      for (int i = 0; i < entry.count(); i+=2) {
        auto sensorId = entry.itemAt(i, Type2Type<MINT32>());
        auto status = entry.itemAt(i+1, Type2Type<MINT32>());
        bool isMaster = (sensorId == previewMaster) ? true : false;
        if (status == MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STREAMING) {
          MY_LOGD("custom zone: streaming sensor(%d)", sensorId);
          updateSensorState(sensorId, isMaster, SensorStatus::E_STREAMING);
        } else if (status == MTK_HALCORE_PHYSICAL_SENSOR_STATUS_STANDBY) {
          updateSensorState(sensorId, isMaster, SensorStatus::E_STANDBY);
        } else {
          updateSensorState(sensorId, isMaster, SensorStatus::E_STANDBY);
        }
      }
      return true;
    } else {
      MY_LOGE("custom zone: invalid sensor status control, meta count(%d)",
              entry.count());
      return false;
    }
  }
}

bool CustomerSensorControl::decideSensorStatus(
        SensorControlParamOut& out,
        SensorControlParamIn const& in) {
  auto entry = in.pAppControl->entryFor(MTK_HALCORE_PHYSICAL_SENSOR_STATUS);
  bool isCustomZone = entry.isEmpty() ? false : true;

  if (isCustomZone) {
    setCustomZoneSensorStatus(out, in);
  } else {
    MY_LOGD("customer implement");
  }
  return true;
}

bool CustomerSensorControl::decideCropRegion(
        SensorControlParamOut& out,
        SensorControlParamIn const& in) {
  MY_LOGD("customer implement");
  return true;
}

bool CustomerSensorControl::decide3ARegion(
        SensorControlParamOut& out,
        SensorControlParamIn const& in) {
  MY_LOGD("customer implement");
  return true;
}

auto
CustomerSensorControl::
doDecision(
    SensorControlParamOut &out __attribute__((unused)),
    SensorControlParamIn const& in
) -> bool
{
    if(in.vSensorIdList.size() == 0) return false;
    // set sensor status (custom zone or user impl.)
    if (!decideSensorStatus(out, in)) {
        return false;
    }
    // set alternactive crop
    if (!decideCropRegion(out, in)) {
        return false;
    }
    // set 3a region
    if (!decide3ARegion(out, in)) {
        return false;
    }

    MRect mFocusROI;
    MINT64 mExposureTime;
    // master id is store in first element.
    MUINT8 mMainSensorId = in.vSensorIdList[0]; // sample code
    (void)mMainSensorId;
    MINT32 mFocusStatus;
    // get share data from staeis plugin
    if(mp3rdReader)
    {
        std::shared_ptr<NSCam::v3::Customer_FOVA_Streaming_Data> data;
        if(mp3rdReader->cloneBack(data))
        {
            MY_LOGD_IF(mLogLevel >= 1,"master(%" PRIu32 ") slave(%" PRIu32 ")", data->masterId, data->slaveId);
            MY_LOGD_IF(mLogLevel >= 1,"focus ROI:%d %d %d %d,mFocusStatus:%d,exposure time:%" PRId64 "",
                data->mFocusROI.p.x,data->mFocusROI.p.y,data->mFocusROI.s.w, data->mFocusROI.s.h,data->mFocusStatus,data->mExposureTime);
            mFocusROI = data->mFocusROI;
            mExposureTime = data->mExposureTime;
            mFocusStatus = data->mFocusStatus;
        }
    }

    return false;
}

auto
CustomerSensorControl::
uninit(
) -> bool
{
    // TODO

    return false;
}

};  //namespace sensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam


