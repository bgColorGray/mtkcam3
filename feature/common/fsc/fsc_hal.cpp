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


#define LOG_TAG "fsc_hal"

#include <cstdio>
#include <utils/SystemClock.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/def/common.h>
#include <cutils/properties.h>  // For property_get().
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/std/Misc.h>
#include <fsc_hal_imp.h>
#include <mtkcam3/feature/fsc/fsc_defs.h>
#include <mtkcam3/feature/fsc/fsc_ext.h>
#include "hal/inc/camera_custom_fsc.h"
#include "hal/inc/camera_custom_eis.h"

CAM_ULOG_DECLARE_MODULE_ID(MOD_FSC_HAL);

#undef FSC_LOG_LV1
#undef FSC_LOG_LV2
#undef FSC_LOG_LV3
#undef FSC_LOG
#undef FSC_DO
#undef MY_LOGD
#undef MY_LOGI
#undef MY_LOGW
#undef MY_LOGE
#undef MY_LOGD0
#undef MY_LOGD1
#undef MY_LOGD2
#undef MY_LOGD3
#undef MY_TRACE_API_LIFE
#undef MY_TRACE_FUNC_LIFE
#undef MY_TRACE_TAG_LIFE
#undef MY_LOGA


#define FSC_LOG_LV1                   ( 1 <= mDebugLevel )
#define FSC_LOG_LV2                   ( 2 <= mDebugLevel )
#define FSC_LOG_LV3                   ( 3 <= mDebugLevel )

#define FSC_LOG(lv, fmt, arg...)      CAM_ULOGM##lv("[%s]" fmt, __FUNCTION__, ##arg)

#define FSC_DO(cmd)                   do { cmd; } while(0)

#define MY_LOGD(fmt, arg...)          FSC_DO( FSC_LOG(D, fmt, ##arg))
#define MY_LOGI(fmt, arg...)          FSC_DO( FSC_LOG(I, fmt, ##arg))
#define MY_LOGW(fmt, arg...)          FSC_DO( FSC_LOG(W, fmt, ##arg))
#define MY_LOGE(fmt, arg...)          FSC_DO( FSC_LOG(E, fmt, ##arg))

#define MY_LOGD_IF(c, fmt, arg...)    FSC_DO( if(c) FSC_LOG(D, fmt, ##arg))
#define MY_LOGI_IF(c, fmt, arg...)    FSC_DO( if(c) FSC_LOG(I, fmt, ##arg))
#define MY_LOGW_IF(c, fmt, arg...)    FSC_DO( if(c) FSC_LOG(W, fmt, ##arg))
#define MY_LOGE_IF(c, fmt, arg...)    FSC_DO( if(c) FSC_LOG(E, fmt, ##arg))

#define MY_LOGD0(fmt, arg...)         MY_LOGD(fmt, ##arg)
#define MY_LOGD1(fmt, arg...)         MY_LOGD_IF(FSC_LOG_LV1, fmt, ##arg)
#define MY_LOGD2(fmt, arg...)         MY_LOGD_IF(FSC_LOG_LV2, fmt, ##arg)
#define MY_LOGD3(fmt, arg...)         MY_LOGD_IF(FSC_LOG_LV3, fmt, ##arg)

#define MY_LOGW0(fmt, arg...)         MY_LOGD(fmt, ##arg)
#define MY_LOGW1(fmt, arg...)         MY_LOGD_IF(FSC_LOG_LV1, fmt, ##arg)

#define MY_TRACE_API_LIFE()           CAM_ULOGM_APILIFE()
#define MY_TRACE_FUNC_LIFE()          CAM_ULOGM_FUNCLIFE()
#define MY_TRACE_TAG_LIFE(name)       CAM_ULOGM_TAGLIFE(name)

#define BIN_REVERT(x)  (x = (x << 1))
#define P1_PORT_BUF_IDX_NONE (0xFFFFFFFF) // MUINT32 (4 bytes with P1_FILL_BYTE)
#define FSC_DUMP_FOLDER_PATH "/data/vendor/camera_dump/fsc"

/*******************************************************************************
*
********************************************************************************/
using NSCam::Type2Type;
using NSCam::MRect;
using NSCam::MPoint;
using NSCam::MSizeF;

using namespace NSCam::FSC;

static MUINT32 sForceEisOn;

/******************************************************************************
 *  Metadata Access
 ******************************************************************************/
template <typename T>
inline MBOOL
tryGetMetadata(
    IMetadata const* pMetadata,
    MUINT32 const tag,
    T & rVal
)
{
    if (pMetadata == NULL) {
        MY_LOGW("pMetadata == NULL");
        return MFALSE;
    }
    //
    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if(!entry.isEmpty()) {
        rVal = entry.itemAt(0, Type2Type<T>());
        return MTRUE;
    }
    //
    return MFALSE;
}

template <typename T>
inline MBOOL
trySetMetadata(
    IMetadata* pMetadata,
    MUINT32 const tag,
    T const& val
)
{
    if (pMetadata == NULL) {
        MY_LOGW("pMetadata == NULL");
        return MFALSE;
    }
    //
    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    if (0 == pMetadata->update(tag, entry)) {
        return MTRUE;
    }
    //
    return MFALSE;
}

template <typename... Args>
static void mySnprintf(char *buf, size_t n, const char *fmt, Args... args)
{
    if( buf && fmt )
    {
        int ret = snprintf(buf, n, fmt, args...);
        MY_LOGE_IF((ret < 0), "snprintf FAIL");
    }
}

static void mySnprintf(char *buf, size_t n, const char *fmt)
{
    if( buf && fmt )
    {
        int ret = snprintf(buf, n, "%s", fmt);
        MY_LOGE_IF((ret < 0), "snprintf FAIL");
    }
}


/*******************************************************************************
*
********************************************************************************/
sp<FSCHal> FSCHal::CreateInstance(char const *userName, MUINT32 sensorIdx)
{
    MY_LOGI("%s sensorIdx %d", userName, sensorIdx);
    return new FSCHalImp(sensorIdx);
}

FSCHal::~FSCHal()
{
}

/*******************************************************************************
*
********************************************************************************/
FSCHalImp::FSCHalImp(MUINT32 sensorIdx)
    : FSCHal()
    , mSensorIdx(sensorIdx)
    , mDebugLevel(0)
    , mFirstFrame(MTRUE)
    , mlastTargetDAC(0)
    , mpFSCInitFp(NULL)
    , mpAFUpdateFp(NULL)
    , mpFSCUpdateFp(NULL)
    , mpFSCProvider(NULL)
    , mlastestExpTime(0)
{
    MY_TRACE_FUNC_LIFE();

    mDebugLevel = FSCCustom::getDebugLevel();
    mpFSCProvider = MTKFSC::createInstance();
    sForceEisOn = ::property_get_int32("vendor.debug.camera.hal3.eis30", 0);
    MY_LOGI("sensorIdx(%d) mDebugLevel(%d)", mSensorIdx, mDebugLevel);
}

FSCHalImp::~FSCHalImp()
{
    MY_TRACE_FUNC_LIFE();

    MY_LOGI("sensorIdx(%d)", mSensorIdx);

    if (mpFSCProvider != NULL)
        mpFSCProvider->destroyInstance();
}

MINT32 FSCHalImp::Init(IHal3A* p3A, MSize sensorSize)
{
    MY_TRACE_API_LIFE();

    MY_LOGI("sensorIdx(%d) size(%d,%d)", mSensorIdx, sensorSize.w, sensorSize.h);

    if (mpFSCProvider == NULL)
    {
        MY_LOGE("mpFSCProvider is NULL!");
        return FSC_HAL_RETURN_NULL_OBJ;
    }

    if (!p3A)
    {
        MY_LOGE("p3A is NULL!");
    }
    else
    {
        p3A->attachCb(IHal3ACb::eID_NOTIFY_AF_FSC_INFO, this);
    }

    mSensorSize = sensorSize;
    if (::property_get_int32("vendor.fsc.af.dump", 0))
    {
        createDumpFile();
    }

    return FSC_HAL_RETURN_NO_ERROR;
}

MINT32 FSCHalImp::Uninit(IHal3A* p3A)
{
    MY_TRACE_API_LIFE();

    MY_LOGI("sensorIdx(%d)", mSensorIdx);
    if (p3A)
    {
        p3A->detachCb(IHal3ACb::eID_NOTIFY_AF_FSC_INFO, this);
    }

    mpFSCInitFp = NULL;
    mpAFUpdateFp = NULL;
    mpFSCUpdateFp = NULL;

    return FSC_HAL_RETURN_NO_ERROR;
}

MINT32 FSCHalImp::getAFInitParams(IHal3A* p3A)
{
    MY_TRACE_FUNC_LIFE();

    if (!p3A)
    {
        MY_LOGE("p3A is NULL!");
        return FSC_HAL_RETURN_NULL_OBJ;
    }
    else
    {
        if (!p3A->send3ACtrl(E3ACtrl_Get_AF_FSC_INIT_INFO, reinterpret_cast<MINTPTR>(&mDacInit), 0))
        {
            MY_LOGE("send3ACtrl E3ACtrl_GET_AF_FSC_INIT_INFO failed!");
            return FSC_HAL_RETURN_UNKNOWN_ERROR;
        }
        MY_LOGD("M_to_I_R(%d) D_I(%d) D_M(%d) D_T(%d) R_T(%d) iI_D(%d) A_Ta_S(%d) A_T_E(%d)",
            mDacInit.macro_To_Inf_Ratio, mDacInit.dac_Inf, mDacInit.dac_Macro,
            mDacInit.damping_Time, mDacInit.readout_Time_us, mDacInit.init_DAC,
            mDacInit.af_Table_Start, mDacInit.af_Table_End);
    }

    return FSC_HAL_RETURN_NO_ERROR;
}

MINT32 FSCHalImp::processResult(
                            MRect crop,// full sensor size domain
                            IMetadata* pInAPP,//inAPP
                            IMetadata* pInHAL __unused,//inHAL
                            MetaSet_T & result3A,
                            IHal3A* p3A,
                            MINT32 const currMagicNum,
                            QBufInfo const &deqBuf,
                            MUINT32 const bufIdxRss,
                            MUINT32 const bufIdxRrz,
                            MUINT32 const index,
                            IMetadata &rOutputFSC
                        )
{
    MY_TRACE_API_LIFE();

    // get AE
    MINT64 exposureTime = 0;
    MINT32 iExpTime = 0;
    if (!tryGetMetadata<MINT64>
        (&result3A.appMeta, MTK_SENSOR_EXPOSURE_TIME, exposureTime)) {
        MY_LOGW("no MTK_SENSOR_EXPOSURE_TIME, use latestExpTime(%d)", mlastestExpTime);
        iExpTime = mlastestExpTime;
    }
    else
    {
        iExpTime = exposureTime/((MINT64)1000);//(ns to us) << frame duration
        mlastestExpTime = iExpTime;
    }

    // get EIS on
    MUINT8 eisMode = MTK_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    if (!tryGetMetadata<MUINT8>(pInAPP,
        MTK_CONTROL_VIDEO_STABILIZATION_MODE, eisMode)) {
        MY_LOGW1("no MTK_CONTROL_VIDEO_STABILIZATION_MODE");
    }
    MINT32 adveisMode = MTK_EIS_FEATURE_EIS_MODE_OFF;
    if (!tryGetMetadata<MINT32>(pInAPP,
        MTK_EIS_FEATURE_EIS_MODE, adveisMode)) {
        MY_LOGW1("no MTK_EIS_FEATURE_EIS_MODE");
    }
    MINT32 previewEis = MTK_EIS_FEATURE_PREVIEW_EIS_OFF;
    if (EISCustom::isSupportPreviewEIS())
    {
        tryGetMetadata<MINT32>(pInAPP, MTK_EIS_FEATURE_PREVIEW_EIS, previewEis);
    }

    if (mFirstFrame)
    {
        MY_LOGD("first frame process magic(%d) ET(%d) EIS(%d), CB magic(%d)", currMagicNum, iExpTime, eisMode,
        mLastFSCFrameInfo.SttNum);
        getAFInitParams(p3A);// get init_DAC after 1st frame

        if (mpFSCProvider)
        {
            // AF:must get init_DAC after 1st frame done
            FSC_INIT_STRUCT fsc_init;
            fsc_init.macro_to_inf_ratio = FSC_MACRO_INF_RATIO_UNIT + mDacInit.macro_To_Inf_Ratio + FSCCustom::getMarcoToInfRatioOffset();
            fsc_init.max_cropped_ratio = FSCCustom::getMaxCropRatio(); // 10% = 0.1 * 10000
            fsc_init.dac_Inf = mDacInit.dac_Inf;
            fsc_init.dac_Macro = mDacInit.dac_Macro;
            fsc_init.af_table_start = mDacInit.af_Table_Start;
            fsc_init.af_table_end = mDacInit.af_Table_End;
            fsc_init.readout_time_mus = mDacInit.readout_Time_us;
            fsc_init.af_time_mus = mDacInit.damping_Time * FSC_DAC_DAMPING_TIME_TO_US + FSCCustom::getAfDampTimeOffset();// us
            if (mpFSCInitFp)
            {
                char content[256] = {0};
                mySnprintf(content, sizeof(content), "%d,%d,%d,%d,%d,%d,%d,%d\n", fsc_init.macro_to_inf_ratio, fsc_init.max_cropped_ratio,fsc_init.dac_Inf,
                    fsc_init.dac_Macro, fsc_init.af_table_start, fsc_init.af_table_end, fsc_init.readout_time_mus, fsc_init.af_time_mus);
                mpFSCInitFp->write(&content, sizeof(content));
            }
            {
                MY_TRACE_TAG_LIFE("FSCProvider::FSCInit");
                if (S_FSC_OK != mpFSCProvider->FSCInit(&fsc_init))
                {
                    MY_LOGW("FSCInit failed!");
                }
            }

            FSC_AF_UPDATE_STRUCT af_upate;
            af_upate.request_frame_num = currMagicNum; // first number could be 2
            af_upate.rolling_shutter_percentage = 0;
            af_upate.target_DAC = mDacInit.init_DAC;
            {
                MY_TRACE_TAG_LIFE("FSCProvider::FSC_FEATURE_SET_AF_UPDATE_INFO");
                mpFSCProvider->FSCFeatureCtrl(FSC_FEATURE_SET_AF_UPDATE_INFO, &af_upate, NULL);
            }
            if (mpAFUpdateFp)
            {
                char content[256] = {0};
                mySnprintf(content, sizeof(content), "%d,%d,%d\n", af_upate.request_frame_num, af_upate.rolling_shutter_percentage, af_upate.target_DAC);
                mpAFUpdateFp->write(&content, sizeof(content));
            }
        }

        mFirstFrame = MFALSE;
    }

    if (mpFSCProvider)
    {
        MY_TRACE_TAG_LIFE("FSCProvider::QUERY");
        // get RRZO info
        NSCam::NSIoPipe::NSCamIOPipe::ResultMetadata const *rrzoResult = NULL;
        NSCam::NSIoPipe::NSCamIOPipe::ResultMetadata const *rssoResult = NULL;
        MSizeF rrzoSizeF, rssoSizeF;

        if (bufIdxRrz != P1_PORT_BUF_IDX_NONE && bufIdxRrz < deqBuf.mvOut.size()) {
            rrzoResult = &(deqBuf.mvOut[bufIdxRrz].mMetaData);
        }
        if (rrzoResult == NULL) {
            MY_LOGE("CANNOT get result at (%d)", bufIdxRrz);
            return FSC_HAL_RETURN_INVALID_PARA;
        }

        // get FSC CROP ratio
        FSC_CROPPING_DATA_STRUCT fsc_crop_data;
        FSC_IMAGE_SCALE_INFO_STRUCT image_scale_input;
        FSC_IMAGE_SCALE_RESULT_STRUCT image_scale_output;
        image_scale_input.request_frame_num = currMagicNum;
        image_scale_input.exposureTime_mus = iExpTime;
        image_scale_input.h_sensor_out = mSensorSize.h;
        image_scale_input.y_sensor_crop = crop.p.y;
        image_scale_input.h_sensor_crop = crop.s.h;
        if (S_FSC_OK != mpFSCProvider->FSCFeatureCtrl(FSC_FEATURE_GET_CROP_REMAIN_SIZE_RATIO, &image_scale_input, &image_scale_output))
        {
            MY_LOGW("FSC_FEATURE_GET_CROP_REMAIN_SIZE_RATIO failed!");
        }

        if (image_scale_output.image_scale == 0)
        {
            MY_LOGW("image_scale_output.image_scale == 0");
            image_scale_output.image_scale = FSC_MAX_SCALING_RATIO;
        }
        // update crop ratio for display and record
        fsc_crop_data.image_scale = image_scale_output.image_scale/FSC_MAX_SCALING_RATIO;
        rrzoSizeF = rrzoResult->mDstSize;
        fsc_crop_data.RRZOFSCRegion.p.x = (rrzoSizeF.w * (1.0f-fsc_crop_data.image_scale) / 2);
        fsc_crop_data.RRZOFSCRegion.p.y = (rrzoSizeF.h * (1.0f-fsc_crop_data.image_scale) / 2);
        fsc_crop_data.RRZOFSCRegion.s   = (rrzoSizeF * fsc_crop_data.image_scale);

        if (mpFSCUpdateFp)
        {
            char content[256] = {0};
            mySnprintf(content, sizeof(content), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", currMagicNum, iExpTime, mSensorSize.w, mSensorSize.h,
                     crop.p.x, crop.p.y, crop.s.w, crop.s.h, (MINT32)rrzoSizeF.w, (MINT32)rrzoSizeF.h);
            mpFSCUpdateFp->write(&content, sizeof(content));
        }

        if (eisMode == MTK_CONTROL_VIDEO_STABILIZATION_MODE_ON ||
            adveisMode == MTK_EIS_FEATURE_EIS_MODE_ON ||
            previewEis == MTK_EIS_FEATURE_PREVIEW_EIS_ON ||
            sForceEisOn)
        {
            FSC_QUERY_INFO_STRUCT rrzo_query_input;
            FSC_WARPING_RESULT_INFO_STRUCT query_warp_output;
            rrzo_query_input.request_frame_num = currMagicNum;
            rrzo_query_input.exposureTime_mus = iExpTime;
            rrzo_query_input.w_sensor_out = mSensorSize.w;
            rrzo_query_input.h_sensor_out = mSensorSize.h;
            rrzo_query_input.x_sensor_crop = crop.p.x;
            rrzo_query_input.y_sensor_crop = crop.p.y;
            rrzo_query_input.w_sensor_crop = crop.s.w;
            rrzo_query_input.h_sensor_crop = crop.s.h;
            rrzo_query_input.w_input = rrzoSizeF.w;
            rrzo_query_input.h_input = rrzoSizeF.h;
            // get warping result for EIS
            if (S_FSC_OK == mpFSCProvider->FSCFeatureCtrl(FSC_FEATURE_GET_WARPING_RESULT_INFO, &rrzo_query_input, &query_warp_output))
            {
                FSC_WARPING_DATA_STRUCT fsc_warp_data;
                fsc_warp_data.fsc_warp_result = query_warp_output;

                FSC_SENSOR_INFO_STRUCT fsc_sensor_info;
                fsc_sensor_info.w_sensor_out = mSensorSize.w;
                fsc_sensor_info.h_sensor_out = mSensorSize.h;
                fsc_sensor_info.x_sensor_crop = crop.p.x;
                fsc_sensor_info.y_sensor_crop = crop.p.y;
                fsc_sensor_info.w_sensor_crop = crop.s.w;
                fsc_sensor_info.h_sensor_crop = crop.s.h;
                fsc_sensor_info.w_input = rrzoSizeF.w;
                fsc_sensor_info.h_input = rrzoSizeF.h;

                FSC_CROP_RESULT_INFO_STRUCT max_query_crop_output;
                if (S_FSC_OK == mpFSCProvider->FSCFeatureCtrl(FSC_FEATURE_GET_MIN_CROP_INFO, &fsc_sensor_info, &max_query_crop_output))
                {
                    fsc_warp_data.maxRRZOCropRegion = MRect(MPoint(max_query_crop_output.x_input_crop, max_query_crop_output.y_input_crop),
                                                        MSize(max_query_crop_output.w_input_crop, max_query_crop_output.h_input_crop));
                }
                else
                {
                    MY_LOGW("EIS: FSC_FEATURE_GET_MIN_CROP_INFO failed!");
                }
                fsc_warp_data.timestamp = deqBuf.mvOut[index].mMetaData.mTimeStamp;
                fsc_warp_data.magicNum = currMagicNum;

                IMetadata::Memory warpData;
                warpData.resize(sizeof(FSC_WARPING_DATA_STRUCT));
                memcpy(warpData.editArray(), &fsc_warp_data, sizeof(FSC_WARPING_DATA_STRUCT));

                IMetadata::IEntry warp_entry(MTK_FSC_WARP_DATA);
                warp_entry.push_back(warpData, Type2Type< IMetadata::Memory >());
                rOutputFSC.update(warp_entry.tag(), warp_entry);
            }
            else
            {
                MY_LOGW("EIS: FSC_FEATURE_GET_WARPING_RESULT_INFO failed!");
            }
        }

        // get RSSO info for 3DNR or EIS
        if (bufIdxRss != P1_PORT_BUF_IDX_NONE && bufIdxRss < deqBuf.mvOut.size()) {
            rssoResult = &(deqBuf.mvOut[bufIdxRss].mMetaData);

            if (rssoResult != NULL)
            {
                // get RSSO crop
                rssoSizeF = rssoResult->mDstSize;
                fsc_crop_data.RSSOFSCRegion.p.x = (rssoSizeF.w * (1.0f-fsc_crop_data.image_scale) / 2);
                fsc_crop_data.RSSOFSCRegion.p.y = (rssoSizeF.h * (1.0f-fsc_crop_data.image_scale) / 2);
                fsc_crop_data.RSSOFSCRegion.s   = (rssoSizeF * fsc_crop_data.image_scale);
            }
        }
        MY_LOGD("P magic(%d) dac(%d) ET(%d) EIS(%d), CB magic(%d), factor(%f), sensor(%d,%d), RRZO: c(%d,%d,%d,%d) s(%.0f,%.0f) f(%f,%f,%f,%f),"
            "RSSO: s(%.0f,%.0f), f(%f,%f,%f,%f)",
            currMagicNum, mlastTargetDAC, iExpTime, eisMode,
            mLastFSCFrameInfo.SttNum, fsc_crop_data.image_scale,
            mSensorSize.w, mSensorSize.h,
            crop.p.x, crop.p.y, crop.s.w, crop.s.h,
            rrzoSizeF.w, rrzoSizeF.h,
            fsc_crop_data.RRZOFSCRegion.p.x, fsc_crop_data.RRZOFSCRegion.p.y, fsc_crop_data.RRZOFSCRegion.s.w, fsc_crop_data.RRZOFSCRegion.s.h,
            rssoSizeF.w, rssoSizeF.h,
            fsc_crop_data.RSSOFSCRegion.p.x, fsc_crop_data.RSSOFSCRegion.p.y, fsc_crop_data.RSSOFSCRegion.s.w, fsc_crop_data.RSSOFSCRegion.s.h);

        IMetadata::Memory cropData;
        cropData.resize(sizeof(FSC_CROPPING_DATA_STRUCT));
        memcpy(cropData.editArray(), &fsc_crop_data, sizeof(FSC_CROPPING_DATA_STRUCT));

        IMetadata::IEntry crop_entry(MTK_FSC_CROP_DATA);
        crop_entry.push_back(cropData, Type2Type< IMetadata::Memory >());
        rOutputFSC.update(crop_entry.tag(), crop_entry);

    }

    return FSC_HAL_RETURN_NO_ERROR;
}

/******************************************************************************
 *
 ******************************************************************************/
void FSCHalImp::doNotifyCb(MINT32  _msgType,MINTPTR _ext1,MINTPTR  /*_ext2*/,MINTPTR /*_ext3*/)
{
    MY_TRACE_API_LIFE();

    switch(_msgType)
    {
        case IHal3ACb::eID_NOTIFY_AF_FSC_INFO:
        {
            FSC_FRM_INFO_T* pOutputParam = (FSC_FRM_INFO_T*)_ext1;
            MY_LOGD_IF(mDebugLevel, "fsc callback magic(%d) num(%d) (%d->%d,%d%%) (%d->%d,%d%%)", pOutputParam->SttNum, pOutputParam->SetCount,
                pOutputParam->DACInfo[0].DAC_From, pOutputParam->DACInfo[0].DAC_To, pOutputParam->DACInfo[0].Percent,
                pOutputParam->DACInfo[1].DAC_From, pOutputParam->DACInfo[1].DAC_To, pOutputParam->DACInfo[1].Percent);
            mLastFSCFrameInfo = (*pOutputParam);

            if (pOutputParam->SetCount)
            {
                for( MINT32 i = pOutputParam->SetCount - 1; i >= 0; i--)
                {
                    FSC_AF_UPDATE_STRUCT af_upate;
                    af_upate.request_frame_num = pOutputParam->SttNum;
                    af_upate.rolling_shutter_percentage = pOutputParam->DACInfo[i].Percent;
                    af_upate.target_DAC = pOutputParam->DACInfo[i].DAC_To;
                    mlastTargetDAC = pOutputParam->DACInfo[i].DAC_To;

                    mpFSCProvider->FSCFeatureCtrl(FSC_FEATURE_SET_AF_UPDATE_INFO, &af_upate, NULL);
                    if (mpAFUpdateFp)
                    {
                        char content[256] = {0};
                        mySnprintf(content, sizeof(content), "%d,%d,%d\n", af_upate.request_frame_num, af_upate.rolling_shutter_percentage, af_upate.target_DAC);
                        mpAFUpdateFp->write(&content, sizeof(content));
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}

void FSCHalImp::createDumpFile()
{
    MUINT64 timewithoutSleep = uptimeMillis();
    char path[256];
    mySnprintf(path, sizeof(path), "%s", FSC_DUMP_FOLDER_PATH);
    if (NSCam::Utils::makePath(path, 0777) == false)
    {
        MY_LOGW("makePath(\"%s\") error", path);
        return;
    }
    MY_LOGD("makePath(\"%s\")", FSC_DUMP_FOLDER_PATH);

    //open file
    mySnprintf(path, sizeof(path), FSC_DUMP_FOLDER_PATH"/%" PRId64 "_FSC_Init.csv", timewithoutSleep);
    mpFSCInitFp = NSCam::IFileCache::open(path);
    if (mpFSCInitFp)
    {
        char content[256] = {0};
        mySnprintf(content, sizeof(content), "macro_to_inf_ratio,max_cropped_ratio,dac_Inf,dac_Macro,"
                "af_table_start,af_table_end,readout_time_mus,af_time_mus\n");
        mpFSCInitFp->write(&content, sizeof(content));
    }
    else
    {
        MY_LOGW("Can't open file %s", path);
    }

    mySnprintf(path, sizeof(path), FSC_DUMP_FOLDER_PATH"/%" PRId64 "_AF_Update.csv", timewithoutSleep);
    mpAFUpdateFp = NSCam::IFileCache::open(path);
    if (mpAFUpdateFp)
    {
        char content[256] = {0};
        mySnprintf(content, sizeof(content), "request_frame_num,rolling_shutter_percentage,target_DAC\n");
        mpAFUpdateFp->write(&content, sizeof(content));
    }
    else
    {
        MY_LOGW("Can't open file %s", path);
    }

    mySnprintf(path, sizeof(path), FSC_DUMP_FOLDER_PATH"/%" PRId64 "_FSC_Update.csv", timewithoutSleep);
    mpFSCUpdateFp = NSCam::IFileCache::open(path);
    if (mpFSCUpdateFp)
    {
        char content[256] = {0};
        mySnprintf(content, sizeof(content), "request_frame_num,exposureTime_mus,w_sensor_out,h_sensor_out,"
            "x_sensor_crop,y_sensor_crop,w_sensor_crop,h_sensor_crop,w_input,h_input\n");
        mpFSCUpdateFp->write(&content, sizeof(content));
    }
    else
    {
        MY_LOGW("Can't open file %s", path);
    }
}


