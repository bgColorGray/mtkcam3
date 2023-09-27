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
/* MediaTek Inc. (C) 2017. All rights reserved.
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

#define LOG_TAG "MtkCam/ISPStreamController"
#include "ISPStreamController.h"
//
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <utility>
// hidl interface
#include "vendor/mediatek/hardware/camera/isphal/1.1/types.h"
//
#include <cutils/properties.h>
#include <hidl/Status.h>
#include <utils/Errors.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
#include <mtkcam/utils/std/common.h>
//#include "mtkcam-interfaces/isphal/IHalISPAdapter.h"
#include <mtkcam3/pipeline/hwnode/StreamId.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
//#include "mtkcam-interfaces/pipeline/utils/packutils/PackUtils_v2.h"
// for fd container
//#include <mtkcam/utils/hw/IFDContainer.h>
// for rrzo temp patch
#include <mtkcam3/feature/bsscore/IBssCore.h>
//#include <mtkcam/utils/imgbuf/IGraphicImageBsufferHeap.h>
//#include "mtkcam-android/main/hal/postproc/StreamId.h"
#include <mtkcam/utils/gralloc/IGrallocHelper.h>
//
#include "IISPManager.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_HAL_SERVER);

namespace NSCam {
namespace ISPHal {

#define MY_LOGV(fmt, arg...) \
  CAM_ULOGMV(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) \
  CAM_ULOGMD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...) \
  CAM_ULOGMI(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) \
  CAM_ULOGMW(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) \
  CAM_ULOGME(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...) \
  CAM_LOGA(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...) \
  CAM_LOGF(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)

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

#if 1
#define FUNC_START MY_LOGD("+")
#define FUNC_END MY_LOGD("-")
#else
#define FUNC_START
#define FUNC_END
#endif
//
#if 1
#define MY_LOG_DEBUG(fmt, arg...) \
  CAM_LOGD(LOG_TAG "(%d)[%s] " fmt "\n", ::gettid(), __FUNCTION__, ##arg)
#else
#define MY_LOG_DEBUG(fmt, arg...) ((void)0)
#endif
//

using IImageStreamInfo = NSCam::v3::IImageStreamInfo;
using ISPStreamBufferHandle =
    NSCam::v3::ISPImageStreamBuffer::ISPStreamBufferHandle;

using android::String8;
using ::android::hardware::hidl_handle;
using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_2::PixelFormat;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using vendor::mediatek::hardware::camera::isphal::V1_1::ISPStreamConfiguration;
using vendor::mediatek::hardware::camera::isphal::V1_1::ISPRealFormat;
using vendor::mediatek::hardware::camera::isphal::V1_1::ISPStream;
using vendor::mediatek::hardware::camera::isphal::V1_1::ISPRequest;
using vendor::mediatek::hardware::camera::isphal::V1_1::ISPBuffer;
//using NSCam::v3::PackUtilV2::IIspTuningDataPackUtil;
//using NSCam::v3::IImageStreamInfoBuilder;
//using mcam::android::CameraBlob;
//using mcam::android::CameraBlobId;

auto ISPStreamController::configure_WPE_1_1(
    const ISPStreamConfiguration& config,
    std::shared_ptr<UserConfigurationParams>& params) -> int {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ISPStreamController::configure_1_1(
    const ISPStreamConfiguration& config,
    std::shared_ptr<UserConfigurationParams>& params,
    int32_t& module,
    int32_t& camId) -> int {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ISPStreamController::createImageStreamInfo_1_1(
    const ISPStream& srcStream,
    const MUINT32& srcStreamType) -> android::sp<IImageStreamInfo> {
  android::sp<IImageStreamInfo> mtkhalStream;
  return mtkhalStream;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ISPStreamController::queryPlanesInfo_1_1(
  const ISPStream& stream,
  NSCam::BufPlanes& planes)
    -> int {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ISPStreamController::parseRequests_1_1(
    const hidl_vec<ISPRequest>& hidl_reqs,
    std::vector<std::shared_ptr<UserRequestParams>>& parsed_reqs,
    std::shared_ptr<UserConfigurationParams>& params) -> int {
  return 0;
}


/******************************************************************************
 *
 ******************************************************************************/
auto
ISPStreamController::
parseRequests_singleHW_1_1(
  const hidl_vec<ISPRequest>& hidl_reqs,
  std::vector<std::shared_ptr<UserRequestParams>>& parsed_reqs,
  std::shared_ptr<UserConfigurationParams>& params) -> int {
  return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
auto ISPStreamController::convertTuningMetaBuffer_1_1(
    const std::string& bufferName,
    const ISPBuffer& streamBuffer,
    int32_t cacheId,
    android::sp<IImageBufferHeap>& pImgBufHeap, /*output*/
    buffer_handle_t& importedTuningBufferHandle,  /*output*/
    std::shared_ptr<ISPStreamBufferHandle>& ispBufferHandle) -> int {
  return 0;
}


}  // namespace ISPHal
}  // namespace NSCam
