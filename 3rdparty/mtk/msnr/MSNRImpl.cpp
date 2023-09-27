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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#define LOG_TAG "MSNRPlugin"
#include "MSNRImpl.h"
//
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Trace.h>
//
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
//
//
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
//
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/std/Format.h>
//
#include <cutils/properties.h>

#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam/utils/metastore/ITemplateRequest.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
//tuning utils
#include <mtkcam/utils/TuningUtils/FileReadRule.h>
// Algo
#include <mtkcam3/feature/msnr/IMsnr.h>
//
#if MTK_CAM_NEW_NVRAM_SUPPORT
#include <camera_custom_nvram.h>
#include <mtkcam/aaa/INvBufUtil.h>
#include <mtkcam/utils/mapping_mgr/cam_idx_mgr.h>
#endif
//
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h> // tuning file naming

using namespace NSCam;
using namespace android;
using namespace std;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::TuningUtils; //dump buffer

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_MSNR);
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#define ASSERT(cond, msg)           do { if (!(cond)) { printf("Failed: %s\n", msg); return; } }while(0)
//
#define ALIGN_CEIL(x,a) (((x) + (a) - 1L) & ~((a) - 1L))

template <class T>
inline bool
tryGetMetadata( IMetadata const *pMetadata, MUINT32 tag, T& rVal )
{
    if(pMetadata == nullptr)
        return false;

    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if(!entry.isEmpty())
    {
        rVal = entry.itemAt(0,Type2Type<T>());
        return true;
    } else {
        MY_LOGW("metadata tag(%d) is not found", tag);
    }
    return false;
}
/******************************************************************************
 *
 ******************************************************************************/
REGISTER_PLUGIN_PROVIDER(Yuv, MsnrPluginProviderImp);
/******************************************************************************
 *
 ******************************************************************************/
MsnrPluginProviderImp::
MsnrPluginProviderImp()
{
    CAM_ULOGM_APILIFE();
    mEnable      = ::property_get_int32("vendor.debug.camera.msnr.enable", -1);
    muDumpBuffer = ::property_get_int32("vendor.debug.camera.msnr.dump", 0);
}

/******************************************************************************
 *
 ******************************************************************************/
MsnrPluginProviderImp::
~MsnrPluginProviderImp()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
const
MsnrPluginProviderImp::Property&
MsnrPluginProviderImp::
property()
{
    CAM_ULOGM_APILIFE();
    static Property prop;
    static bool inited = false;

    if (!inited) {
        prop.mName = "MTK MSNR";
        prop.mFeatures = MTK_FEATURE_NR;
        prop.mInPlace = MFALSE;
        prop.mFaceData = eFD_None;
        prop.mPosition = 0;
        prop.mPriority = 4;
        inited = true;
    }

    return prop;
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
MsnrPluginProviderImp::
negotiate(Selection& sel)
{
    CAM_ULOGM_APILIFE();

    /* TODO: decide to do or not to do
       iso: </>:no/yes
       feature delay nr: T/F: yes but noP2A(yuv in) /yes and P2A(raw in)
    */
    auto msnrMode = MsnrMode_Disable;

    bool isDynamicMetaValid = sel.mIMetadataDynamic.getControl() != NULL;
    bool isHALMetaValid = sel.mIMetadataHal.getControl() != NULL;
    bool isAppMetaValid = sel.mIMetadataApp.getControl() != NULL;
    if (isDynamicMetaValid && isHALMetaValid && isAppMetaValid) {
        IMetadata* pAppMetaDynamic = sel.mIMetadataDynamic.getControl().get();
        IMetadata* pHalMeta = sel.mIMetadataHal.getControl().get();
        IMetadata* pAppMeta = sel.mIMetadataApp.getControl().get();

        MY_LOGD("get openid:%d", sel.mSensorId);

        IMsnr::ModeParams param = {
            .openId         = sel.mSensorId,
            .isRawInput     = sel.mIsRawInput,
            .isHidlIsp      = sel.mIsHidlIsp,
            .isPhysical     = sel.mIsPhysical,
            .isDualMode     = sel.mIsDualMode,
            .isBayerBayer   = sel.mIsBayerBayer,
            .isBayerMono    = sel.mIsBayerMono,
            .isMaster       = sel.mIsMasterIndex,
            .multiframeType = sel.mMultiframeType,
            .needRecorder   = MTRUE,
            .pHalMeta       = pHalMeta,
            .pDynMeta       = pAppMetaDynamic,
            .pAppMeta       = pAppMeta,
            .pSesMeta       = &(sel.mAppSessionMeta)
        };

        msnrMode = IMsnr::getMsnrMode(param);
        // write execution log to recorder
        ResultParam resParam = {};
        initResultParam(param.openId, *param.pHalMeta, &resParam);
        WRITE_EXECUTION_RESULT_INTO_FILE(resParam, "LPNR is %s(%d)",
            (msnrMode == MsnrMode_Disable)? "OFF" :"ON",msnrMode);

        if(msnrMode == MsnrMode_Disable) {
            MY_LOGD("turn off msnr");
            return -EINVAL;
        }

        if(sel.mMultiCamFeatureMode == MTK_MULTI_CAM_FEATURE_MODE_VSDOF && sel.mFovBufferSize != MSize(0, 0)) {
            mRequestSize = sel.mFovBufferSize;
            MY_LOGD("vsdof request fov size: (%dx%d)", mRequestSize.w, mRequestSize.h);
        } else if(sel.mFullBufferSize != MSize(0, 0)) {
            mRequestSize = sel.mFullBufferSize;
            MY_LOGD("request sensor size: (%dx%d)", mRequestSize.w, mRequestSize.h);
        } else {
            MY_LOGF("cannot get SENSOR SIZE");
            return -EINVAL;
        }
    } else {
        // metadata pointers are abnormal, stop negotiate
        MY_LOGA("get null metadata ptr (Dyn:%d, HAL:%d, App:%d)", isDynamicMetaValid, isHALMetaValid, isAppMetaValid);
    }

    // scale 0
    sel.mIBufferFull
        .setRequired(true)
        .addAcceptedFormat(eImgFmt_MTK_YUV_P010)
        .addAcceptedSize(eImgSize_Full);

    // scale 1, it's only needed in rawmode
    if (msnrMode == MsnrMode_Raw) {
        MSize mssSize = MSize( ALIGN_CEIL(mRequestSize.w, 4) / 2, ALIGN_CEIL(mRequestSize.h, 4) / 2);
        sel.mIBufferResized
            .setRequired(true)
            .addAcceptedFormat(eImgFmt_MTK_YUV_P010)
            .addAcceptedSize(eImgSize_Specified).setSpecifiedSize(mssSize);
        if (sel.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) // Normal MSNR
            sel.mIBufferResized.setCapability(eBufCap_MSS);
        else
            sel.mIBufferResized.setCapability(eBufCap_IMGO);
    }

    // inplace I/O buffer or not
    if (msnrMode == MsnrMode_Raw) {
        if (sel.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) {
            sel.mInPlace = MTRUE;
        } else {
            if (sel.mIsPhysical) {
                sel.mInPlace = MTRUE;
            } else {
                sel.mInPlace = MFALSE;
                MY_LOGD("Turn off InPlace due to VSDOF in HAL");
            }
        }
    } else {
        sel.mInPlace = MFALSE;
    }

    // output
    sel.mOBufferFull
        .setRequired(true)
        .addAcceptedFormat(eImgFmt_MTK_YUV_P010)
        .addAcceptedSize(eImgSize_Full);

    // lcso
    sel.mIBufferLCS
        .setRequired(true);
    // lcesho
    sel.mIBufferLCESHO
        .setRequired(true);

    sel.mIMetadataDynamic.setRequired(true);
    sel.mIMetadataApp.setRequired(true);
    sel.mIMetadataHal.setRequired(true);
    sel.mOMetadataApp.setRequired(false);
    sel.mOMetadataHal.setRequired(true);

    // if true, when main and sub cam are both on,
    // plugin will do process twice. (first is main, second is sub)
    // if false, sub image will be ignored.
    if (sel.mIsMasterIndex && sel.mMultiCamFeatureMode != MTK_MULTI_CAM_FEATURE_MODE_VSDOF) {
        sel.mIsFeatureSupportSub = MTRUE;
    } else {
        sel.mIsFeatureSupportSub = MFALSE;
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
void
MsnrPluginProviderImp::
init()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
MsnrPluginProviderImp::
process(RequestPtr pRequest,
        RequestCallbackPtr pCallback)
{
    CAM_ULOGM_APILIFE();

    // 0.check data
    enum MsnrMode msnrMode = MsnrMode_Disable;
    MINT32 openID = pRequest->mSensorId;
    MY_LOGD("get openid:%d", openID);

    IMetadata* pAppMeta        = nullptr;
    IMetadata* pHalMeta        = nullptr;
    IMetadata* pAppMetaDynamic = nullptr;
    IMetadata* pOutHaMeta      = nullptr;

    IImageBuffer* pIImgBuffer      = nullptr;
    IImageBuffer* pIImageBufferMss = nullptr;
    IImageBuffer* pILCSBuffer      = nullptr;
    IImageBuffer* pILCESHOBuffer   = nullptr;
    IImageBuffer* pOImgBuffer      = nullptr;
    void*         pBufLCE2CALTMVA  = nullptr;

    // check and take out data
    bool ret = true;
    ret &= checkMeta(&pAppMeta       , pRequest->mIMetadataApp    , "IN" , "APP");
    ret &= checkMeta(&pHalMeta       , pRequest->mIMetadataHal    , "IN" , "HAL");
    ret &= checkMeta(&pAppMetaDynamic, pRequest->mIMetadataDynamic, "IN" , "Dynamic");
    // output hal metadata is not always needed.
    checkMeta(&pOutHaMeta     , pRequest->mOMetadataHal    , "OUT", "HAL");

    if(!ret) {
        MY_LOGF("some of meta buffers are null");
        return BAD_VALUE;
    }

    IMsnr::DecideModeParams decideparam = {
        .openId         = openID,
        .isRawInput     = pRequest->mIsRawInput,
        .multiframeType = pRequest->mMultiframeType,
        .needRecorder   = MFALSE,
        .pHalMeta       = pHalMeta,
        .pAppMeta       = pAppMeta
    };

    msnrMode = IMsnr::decideMsnrMode(decideparam);
    if (msnrMode == MsnrMode_Disable) {
        MY_LOGF("msnr mode is off");
        return BAD_VALUE;
    }

    ret &= checkImg(&pIImgBuffer     , pRequest->mIBufferFull   , "IN" , "FULL");
    ret &= checkImg(&pOImgBuffer     , pRequest->mOBufferFull   , "OUT", "FULL");
    if (msnrMode == MsnrMode_Raw) {
        ret &= checkImg(&pIImageBufferMss, pRequest->mIBufferResized, "IN" , "Resize");
        ret &= checkImg(&pILCSBuffer     , pRequest->mIBufferLCS    , "IN" , "LCSO");
        ret &= checkImg(&pILCESHOBuffer  , pRequest->mIBufferLCESHO , "IN" , "LCESHO");

        if (CC_LIKELY(pRequest->mpBufLCE2CALTMVA != nullptr)) {
            MY_LOGD("[IN] L2C VA: 0x%p", pRequest->mpBufLCE2CALTMVA);
            pBufLCE2CALTMVA = pRequest->mpBufLCE2CALTMVA;
        } else {
            MY_LOGE("[IN] L2C is nullptr");
        }
    }

    if(!ret) {
        MY_LOGF("some of image buffers are null");
        return BAD_VALUE;
    }

    auto pMsnr = IMsnr::createInstance(msnrMode);
    if(pMsnr.get() == nullptr) {
        MY_LOGF("msnr core is null");
        return BAD_VALUE;
    }

    MY_LOGD("isVSDOF:%d isMasterID:%d", pRequest->mIsVSDoFMode, pRequest->mIsMasterIndex);

    if(msnrMode == MsnrMode_Raw /*RAW in*/) {
        IMsnr::ConfigParams cfg;
        cfg.openId          = openID;
        cfg.buffSize        = pIImgBuffer->getImgSize();
        cfg.isPhysical      = pRequest->mIsPhysical;
        cfg.isDualMode      = pRequest->mIsDualMode;
        cfg.isBayerBayer    = pRequest->mIsBayerBayer;
        cfg.isBayerMono     = pRequest->mIsBayerMono;
        cfg.isMasterIndex   = pRequest->mIsMasterIndex;
        cfg.hasDCE          = pRequest->mHasDCE;
        cfg.appMeta         = pAppMeta;
        cfg.halMeta         = pHalMeta;
        cfg.appDynamic      = pAppMetaDynamic;
        cfg.inputBuff       = pIImgBuffer;
        cfg.inputRSZBuff    = pIImageBufferMss;
        cfg.inputLCSBuff    = pILCSBuffer;
        cfg.inputLCESHOBuff = pILCESHOBuffer;
        cfg.outputBuff      = pOImgBuffer;
        cfg.bufLce2Caltm    = pBufLCE2CALTMVA;
        cfg.isVSDoFMode     = pRequest->mIsVSDoFMode;
        cfg.fovBufferSize   = pRequest->mFovBufferSize;
        cfg.fovCropRegion   = pRequest->mFovCropRegion;

        if(muDumpBuffer) {
            // input dump
            bufferDump(pHalMeta, pIImgBuffer     , openID, YUV_PORT_UNDEFINED, "msnr-input");
            bufferDump(pHalMeta, pIImageBufferMss, openID, YUV_PORT_UNDEFINED, "msnr-rszinput");
        }
        pMsnr->init(cfg);
        MY_LOGD("Msnr process+");
        pMsnr->doMsnr();
        MY_LOGD("Msnr process-");

        if(muDumpBuffer) {
            // output dump
            bufferDump(pHalMeta, pOImgBuffer     , openID, YUV_PORT_UNDEFINED, "msnr-output");
        }
    } else if(msnrMode == MsnrMode_YUV /*YUV in*/) {
        IMsnr::ConfigParams cfg;
        cfg.openId          = openID;
        cfg.buffSize        = pIImgBuffer->getImgSize();
        cfg.isPhysical      = pRequest->mIsPhysical;
        cfg.isDualMode      = pRequest->mIsDualMode;
        cfg.isBayerBayer    = pRequest->mIsBayerBayer;
        cfg.isBayerMono     = pRequest->mIsBayerMono;
        cfg.isMasterIndex   = pRequest->mIsMasterIndex;
        cfg.appMeta         = pAppMeta;
        cfg.halMeta         = pHalMeta;
        cfg.appDynamic      = pAppMetaDynamic;
        cfg.inputBuff       = pIImgBuffer;
        cfg.outputBuff      = pOImgBuffer;

        if(muDumpBuffer) {
            // input dump
            bufferDump(pHalMeta, pIImgBuffer     , openID, YUV_PORT_UNDEFINED, "yuvmsnr-input");
        }
        pMsnr->init(cfg);
        MY_LOGD("YuvMsnr process+");
        pMsnr->doMsnr();
        MY_LOGD("YuvMsnr process-");

        if(muDumpBuffer) {
            // output dump
            bufferDump(pHalMeta, pOImgBuffer     , openID, YUV_PORT_UNDEFINED, "yuvmsnr-output");
        }
    } else {
        MY_LOGW("unknown msnr mode:%d", msnrMode);
    }

    {
        //release msnrcore
        pMsnr = nullptr;

        //release all the buffer & meta from buffer handle
        #define RELEASE_DATA(buf)\
        if(buf != nullptr)\
            buf->release();

        RELEASE_DATA(pRequest->mIBufferFull);
        RELEASE_DATA(pRequest->mIBufferResized);
        RELEASE_DATA(pRequest->mIBufferLCS);
        RELEASE_DATA(pRequest->mIBufferLCESHO);
        RELEASE_DATA(pRequest->mOBufferFull);
        RELEASE_DATA(pRequest->mIMetadataApp);
        RELEASE_DATA(pRequest->mIMetadataHal);
        RELEASE_DATA(pRequest->mIMetadataDynamic);
        RELEASE_DATA(pRequest->mOMetadataHal);
    }

    if(pCallback.get() != nullptr) {
        pCallback->onCompleted(pRequest, 0);
    }
    return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
void
MsnrPluginProviderImp::
abort(vector<RequestPtr>& pRequests)
{
    CAM_ULOGM_APILIFE();
    // TODO
}

/******************************************************************************
 *
 ******************************************************************************/
void
MsnrPluginProviderImp::
uninit()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
bool
MsnrPluginProviderImp::
checkImg(IImageBuffer** bufptr, BufferHandle::Ptr bufhandle, const char* io, const char* name)
{
    CAM_ULOGM_FUNCLIFE();
    if(CC_LIKELY(bufhandle != nullptr)){
        *bufptr = bufhandle->acquire();
        if(CC_LIKELY(*bufptr != nullptr)){
            MY_LOGD("[%s] %s image VA: 0x%p size:(%d,%d) fmt: 0x%x", io, name,
                    (void*)(*bufptr)->getBufVA(0), (*bufptr)->getImgSize().w,
                    (*bufptr)->getImgSize().h, (*bufptr)->getImgFormat());
            return true;
        } else {
            MY_LOGE("[%s] %s image is null", io, name);
        }
    } else {
        MY_LOGW("[%s] %s image handle is null", io, name);
    }
    return false;
}

/******************************************************************************
 *
 ******************************************************************************/
bool
MsnrPluginProviderImp::
checkMeta(IMetadata** metaptr, MetadataHandle::Ptr metahandle, const char* io, const char* name)
{
    CAM_ULOGM_FUNCLIFE();
    if(CC_LIKELY(metahandle != nullptr)){
        *metaptr = metahandle->acquire();
        if(CC_LIKELY(*metaptr != nullptr)){
            MY_LOGD("[%s] %s metadata count: %d", io, name, (*metaptr)->count());
            return true;
        } else {
            MY_LOGE("[%s] %s metadata is null", io, name);
        }
    } else {
        MY_LOGW("[%s] %s metadata handle is null", io, name);
    }
    return false;
}

/******************************************************************************
 *
 ******************************************************************************/
void
MsnrPluginProviderImp::
bufferDump(const IMetadata *halMeta, IImageBuffer* buff, MINT32 openId, YUV_PORT type, const char *pUserString)
{
    CAM_ULOGM_FUNCLIFE();
    // dump input buffer
    char                      fileName[512] = "";
    FILE_DUMP_NAMING_HINT     dumpNamingHint = {};
    //
    MUINT8 ispProfile = NSIspTuning::EIspProfile_Capture;

    if(!halMeta || !buff) {
        MY_LOGF("HalMeta or buff is nullptr, dump fail");
        return;
    }

    if (!IMetadata::getEntry<MUINT8>(halMeta, MTK_3A_ISP_PROFILE, ispProfile)) {
        MY_LOGW("cannot get ispProfile");
    }

    // Extract hal metadata and fill up file name;
    extract(&dumpNamingHint, halMeta);
    // Extract buffer information and fill up file name;
    extract(&dumpNamingHint, buff);
    // Extract by sensor id
    extract_by_SensorOpenId(&dumpNamingHint, openId);
    // IspProfile
    dumpNamingHint.IspProfile = ispProfile; //EIspProfile_Capture;

    genFileName_YUV(fileName, sizeof(fileName), &dumpNamingHint, type, pUserString);
    buff->saveToFile(fileName);
}

void
MsnrPluginProviderImp::
initResultParam(const MINT32 &openId, const IMetadata &halMeta, ResultParam *resParm)
{
  MINT32 UniqueKey = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_PIPELINE_UNIQUE_KEY, UniqueKey)) {
      MY_LOGW("cannot get UniqueKey");
  }

  MINT32 RequestNo = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_PIPELINE_REQUEST_NUMBER, RequestNo)) {
      MY_LOGW("cannot get RequestNo");
  }

  MINT32 MagicNo = 0;
  if (!IMetadata::getEntry<MINT32>(&halMeta, MTK_P1NODE_PROCESSOR_MAGICNUM, MagicNo)) {
      MY_LOGW("cannot get MagicNo");
  }

  DebugSerialNumInfo &dbgNumInfo = resParm->dbgNumInfo;
  dbgNumInfo.uniquekey = UniqueKey;
  dbgNumInfo.reqNum = RequestNo;
  dbgNumInfo.magicNum = MagicNo;
  resParm->sensorId = openId;
  resParm->moduleId = NSCam::Utils::ULog::MOD_LIB_MSNR;
  resParm->decisionType = DECISION_FEATURE;
  resParm->stageId = -1;  // Defined in atms in ISP7. In ISP6S, it is defined in profile mapper's header.
  resParm->isCapture = true;
  // decide to write execution log to headline
  resParm->writeToHeadline = true;

  return;
}