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
#include <mtkcam/def/common.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/common.h>
#include <memory>
#include "../../entry/legacy/module/LegacyCameraModule.h"
CAM_ULOG_DECLARE_MODULE_ID(MOD_CAMERA_DEVICE);
namespace {
std::unique_ptr<NSCam::Utils::ULog::ULogInitializer> ulogInit;
}
/******************************************************************************
 *
 ******************************************************************************/
#define LOG_TAG "MtkCam/module"
#define MY_LOGV(fmt, arg...) CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGV(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGD_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGD(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGI_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGI(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGW_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGW(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGE_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGE(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGA_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGA(__VA_ARGS__);   \
    }                         \
  } while (0)
#define MY_LOGF_IF(cond, ...) \
  do {                        \
    if ((cond)) {             \
      MY_LOGF(__VA_ARGS__);   \
    }                         \
  } while (0)
/******************************************************************************
 *
 ******************************************************************************/
extern "C" NSCam::legacy_dev3::LegacyCameraModule* getLegacyCameraModule() {
  static NSCam::CameraDeviceManagerImpl singleton("legacy");
  static bool init = singleton.initialize();
  if (!init) {
    MY_LOGE("CameraDeviceManagerImpl::initialize fail %p", &singleton);
    return nullptr;
  }
  static NSCam::legacy_dev3::LegacyCameraModule* module =
      createLegacyCameraModule(&singleton);
  if (module == nullptr) {
    MY_LOGE("LegacyCameraModule is null: %p", module);
    return nullptr;
  }
  return module;
}
////////////////////////////////////////////////////////////////////////////////
//  Implementation of hw_module_methods_t
////////////////////////////////////////////////////////////////////////////////
static int open_device(hw_module_t const* module,
                       const char* name,
                       hw_device_t** device) {
  // return  getLegacyCameraModule()->open(device, module, name);
#warning "set device_version to 0 to avoid build error"
  // device_version = 0 would query info every time
  return getLegacyCameraModule()->open(device, module, name, 0);
}
static hw_module_methods_t* get_module_methods() {
  static hw_module_methods_t _methods = {.open = open_device};
  return &_methods;
}
////////////////////////////////////////////////////////////////////////////////
//  Implementation of camera_module_t
////////////////////////////////////////////////////////////////////////////////
static int get_number_of_cameras(void) {
  return getLegacyCameraModule()->getNumberOfDevices();
}
static int get_camera_info(int cameraId, camera_info* info) {
  MY_LOGD("+ cameraId(%d) camera_info(%p)", cameraId, info);
  return getLegacyCameraModule()->getDeviceInfo(cameraId, *info);
}
static int set_callbacks(camera_module_callbacks_t const* callbacks) {
  MY_LOGD("+ callbacks(%p)", callbacks);
  return getLegacyCameraModule()->setCallback(callbacks);
}
static void get_vendor_tag_ops(vendor_tag_ops_t* ops) {
  getLegacyCameraModule()->getVendorTagOps(ops);
}
static int open_legacy(const struct hw_module_t* module,
                       const char* id,
                       uint32_t halVersion,
                       struct hw_device_t** device) {
  return getLegacyCameraModule()->open(device, module, id, halVersion);
}
static int set_torch_mode(const char* camera_id, bool enabled) {
  auto err = getLegacyCameraModule()->setTorchMode(atoi(camera_id), enabled);
  return (err == -EOPNOTSUPP) ? -ENOSYS : err;
}
static int init() {
  if (!ulogInit) {
    MY_LOGD("+ ULog Initializer");
    ulogInit = std::make_unique<NSCam::Utils::ULog::ULogInitializer>();
  }
  return 0;
}
static int get_physical_camera_info(int physical_camera_id,
                                    camera_metadata_t** static_metadata) {
  return getLegacyCameraModule()->getPhysicalCameraInfo(physical_camera_id,
                                                        static_metadata);
}
static int is_stream_combination_supported(
    int camera_id,
    const camera_stream_combination_t* streams) {
  return getLegacyCameraModule()->isStreamCombinationSupported(camera_id,
                                                               streams);
}
static void notify_device_state_change(uint64_t deviceState) {
  getLegacyCameraModule()->notifyDeviceStateChange(deviceState);
}
/******************************************************************************
 * Implementation of camera_module
 ******************************************************************************/
camera_module_t HAL_MODULE_INFO_SYM __attribute__((visibility("default"))) = {
  .common = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = CAMERA_MODULE_API_VERSION_2_5,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = CAMERA_HARDWARE_MODULE_ID,
    .name = "MediaTek Camera Module",
    .author = "MediaTek",
    .methods = get_module_methods(),
    .dso = NULL,
    .reserved = {0},
  },
  .get_number_of_cameras = get_number_of_cameras,
  .get_camera_info = get_camera_info,
  .set_callbacks = set_callbacks,
  .get_vendor_tag_ops = get_vendor_tag_ops,
  .open_legacy = NULL,
  .set_torch_mode = set_torch_mode,
  .init = init,
  .get_physical_camera_info = get_physical_camera_info,
  .is_stream_combination_supported = is_stream_combination_supported,
  .notify_device_state_change = notify_device_state_change,
  .reserved = {0},
};
