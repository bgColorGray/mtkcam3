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

#ifndef _MTK_CAMERA_INCLUDE_STEREO_DEPTHMAP_FEATUREPIPE_COMMON_H_
#define _MTK_CAMERA_INCLUDE_STEREO_DEPTHMAP_FEATUREPIPE_COMMON_H_

// Standard C header file
#include <ctime>
#include <chrono>
#include <bitset>
#include <aee.h>
// Android system/core header file
#include <utils/RefBase.h>
#include <utils/KeyedVector.h>

// mtkcam custom header file

// mtkcam global header file
#include <mtkcam/def/common.h>
#include <mtkcam3/feature/stereo/pipe/vsdof_common.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam3/feature/stereo/pipe/IDualFeatureRequest.h>

// Module header file
#include <mtkcam/drv/iopipe/Port.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/feature/stereo/hal/stereo_size_provider.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
// Local header file

// Logging log header
#define PIPE_MODULE_TAG "DepthMapPipe"
#define PIPE_CLASS_TAG "Common"
#include <PipeLog.h>

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using android::String8;
using android::KeyedVector;
using namespace android;
using namespace NSCam::NSCamFeature::NSDualFeature;

/*******************************************************************************
* Const Definition
********************************************************************************/
const int VSDOF_CONST_FE_EXEC_TIMES = 2;
const int VSDOF_DEPTH_P2FRAME_SIZE = 10 + 2; // P2A + P2ABayer
const int VSDOF_WORKING_BUF_SET = 3;
const int ISP_TUNING_FE_PASS_MODE= 8;
/*******************************************************************************
* Macro Definition
********************************************************************************/
#undef MY_LOGV_IF
#undef MY_LOGD_IF
#undef MY_LOGI_IF
#undef MY_LOGW_IF
#undef MY_LOGE_IF

#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)

#define AEE_ASSERT(fmt, arg...)\
    do {\
        android::String8 const str = android::String8::format(fmt, ##arg);\
        MY_LOGE("ASSERT(%s) fail", str.string());\
        CAM_ULOG_FATAL(\
            NSCam::Utils::ULog::MOD_SFPIPE_DEPTH,\
            "mtkcam/DepthMapPipe", \
            str.string()); \
        raise(SIGKILL);\
    } while(0)

#define DEFINE_CONTAINER_LOGGING(funcIdx, containerMap)\
    std::string ids="";\
    auto fn##funcIdx = [&]() -> const char*{\
                    for(auto index=0;index<containerMap.size();index++)\
                        ids += std::to_string(containerMap.keyAt(index)) + "|";\
                    return ids.c_str();};

#define CONT_LOGGING(funcIdx)\
    fn##funcIdx()

#define NODE_SIGNAL_ERROR_GUARD()\
    if(mpNodeSignal->getStatus(NodeSignal::STATUS_IN_ERROR))\
    {\
        MY_LOGE("%s : Node Signal error occur!!!", getName());\
        return MFALSE;\
    }\

/******************************************************************************
* Enum Definition
********************************************************************************
/**
  * @brief Buffer ID inside DepthMapPipe
 */
typedef enum DepthMapNode_BufferDataTypes{
    BID_INVALID = PUBLIC_PBID_START,

// ======== public id section start ==========

    // input buffer
    BID_P2A_IN_FSRAW1           = PBID_IN_FSRAW1,
    BID_P2A_IN_FSRAW2           = PBID_IN_FSRAW2,
    BID_P2A_IN_RSRAW1           = PBID_IN_RSRAW1,
    BID_P2A_IN_RSRAW2           = PBID_IN_RSRAW2,
    BID_P2A_IN_LCSO1            = PBID_IN_LCSO1,
    BID_P2A_IN_LCSO2            = PBID_IN_LCSO2,
    BID_P2A_IN_LCSHO1           = PBID_IN_LCSHO1,
    BID_P2A_IN_YUV1             = PBID_IN_YUV1,
    BID_P2A_IN_YUV2             = PBID_IN_YUV2,
    BID_P1_OUT_RECT_IN1         = PBID_IN_P1YUV1,
    BID_P1_OUT_RECT_IN2         = PBID_IN_P1YUV2,
    // output buffer
    BID_P2A_OUT_MV_F            = PBID_OUT_MV_F,
    BID_P2A_OUT_MV_F_CAP        = PBID_OUT_MV_F_CAP,
    BID_P2A_OUT_FDIMG           = PBID_OUT_FDIMG,
    BID_GF_OUT_DMBG             = PBID_OUT_DMBG,
    BID_DLDEPH_DVS_OUT_DEPTH    = PBID_OUT_DEPTHMAP,
    BID_DVS_OUT_UNPROCESS_DEPTH = PBID_OUT_DEPTHMAP,
    BID_P2A_OUT_POSTVIEW        = PBID_OUT_POSTVIEW,
    BID_GF_IN_DEBUG             = PBID_OUT_DEBUG,

    // metadata
    BID_META_IN_APP             = PBID_IN_APP_META,
    BID_META_IN_HAL_MAIN1       = PBID_IN_HAL_META_MAIN1,
    BID_META_IN_HAL_MAIN2       = PBID_IN_HAL_META_MAIN2,
    BID_META_IN_P1_RETURN       = PBID_IN_P1_RETURN_META,
    BID_META_OUT_APP            = PBID_OUT_APP_META,
    BID_META_OUT_HAL            = PBID_OUT_HAL_META,

// ======== public id section end ==========

    // queued metadata (used in QUEUED_DEPTH flow)
    BID_META_IN_APP_QUEUED = PUBLIC_PBID_END,  //100
    BID_META_IN_HAL_MAIN1_QUEUED,
    BID_META_IN_HAL_MAIN2_QUEUED,
    BID_META_OUT_APP_QUEUED,
    BID_META_OUT_HAL_QUEUED,
    BID_META_IN_P1_RETURN_QUEUED,
    // statis buffers
    BID_P2A_WARPMTX_X_MAIN1,
    BID_P2A_WARPMTX_Y_MAIN1,
    BID_P2A_WARPMTX_Z_MAIN1,
    BID_P2A_WARPMTX_X_MAIN2,
    BID_P2A_WARPMTX_Y_MAIN2,
    BID_P2A_WARPMTX_Z_MAIN2,
    BID_N3D_OUT_WARPMTX_MAIN1_X,
    BID_N3D_OUT_WARPMTX_MAIN1_Y,
    BID_N3D_OUT_WARPMTX_MAIN1_Z,
    BID_N3D_OUT_MASK_M_STATIC,
    // internal P2A buffers
    BID_P2A_FE1B_INPUT,
    BID_P2A_FE1B_INPUT_NONSLANT,
    BID_P2A_FE2B_INPUT,
    BID_P2A_FE1C_INPUT,
    BID_P2A_FE2C_INPUT,
    BID_P2A_TUNING,
    // P2A output
    BID_P2A_OUT_FE1AO,
    BID_P2A_OUT_FE2AO,
    BID_P2A_OUT_FE1BO,
    BID_P2A_OUT_FE2BO,
    BID_P2A_OUT_FE1CO,
    BID_P2A_OUT_FE2CO,
    BID_P2A_OUT_RECT_IN1,
    BID_P2A_OUT_RECT_IN2,
    BID_P2A_OUT_FMAO_LR, //20
    BID_P2A_OUT_FMAO_RL,
    BID_P2A_OUT_FMBO_LR,
    BID_P2A_OUT_FMBO_RL,
    BID_P2A_OUT_FMCO_LR,
    BID_P2A_OUT_FMCO_RL,
    BID_P2A_OUT_MY_S,
    BID_P2A_INTERNAL_IN_YUV1,
    BID_P2A_INTERNAL_IN_YUV2,
    BID_P2A_INTERNAL_IN_RRZ2,
    BID_P2A_INTERNAL_IMG3O,
    BID_P2A_INTERNAL_FD,
    // N3D output
    BID_N3D_OUT_MV_Y,
    BID_N3D_OUT_MASK_M,
    BID_N3D_OUT_WARPMTX_MAIN2_X,
    BID_N3D_OUT_WARPMTX_MAIN2_Y,
    // WPE
    BID_WPE_IN_MASK_S,
    BID_WPE_OUT_SV_Y,
    BID_WPE_OUT_MASK_S,
    // === DPE section ===
    // DVS
    BID_DVS_OUT_DV_LR,
    BID_OCC_OUT_CFM_M,
    BID_OCC_OUT_CFM_M_NONSLANT,
    BID_OCC_OUT_NOC_M,
    BID_OCC_OUT_NOC_M_NONSLANT,
    BID_ASF_OUT_CRM,
    BID_ASF_OUT_RD,
    BID_ASF_OUT_HF,
    BID_WMF_OUT_DMW,
    // GF
    BID_GF_INTERNAL_DMBG,
    BID_GF_INTERNAL_DEPTHMAP,
    // DLDEPTH
    BID_DLDEPTH_INTERNAL_DEPTHMAP,
    BID_DLDEPTH_INTERNAL_DEPTHMAP_NONSLANT,
    // SLANT
    BID_SLANT_INTERNAL_DEPTHMAP,

    BID_PQ_PARAM,
    BID_DP_PQ_PARAM,

    BID_DEFAULT_WARPMAP_X,
    BID_DEFAULT_WARPMAP_Y,
#ifdef GTEST
    // UT output
    BID_FE2_HWIN_MAIN1,
    BID_FE2_HWIN_MAIN2,
    BID_FE3_HWIN_MAIN1,
    BID_FE3_HWIN_MAIN2,
#endif

} InternalDepthMapBufferID;

/**
  * @brief Node ID inside the DepthMapPipe
 */
typedef enum eDepthMapPipeNodeID {
    eDPETHMAP_PIPE_NODEID_P2A,
    eDPETHMAP_PIPE_NODEID_P2ABAYER,
    eDPETHMAP_PIPE_NODEID_N3D,
    eDPETHMAP_PIPE_NODEID_DVS,
    eDPETHMAP_PIPE_NODEID_DVP,
    eDPETHMAP_PIPE_NODEID_GF,
    eDPETHMAP_PIPE_NODEID_WPE,
    eDPETHMAP_PIPE_NODEID_DLDEPTH,
    eDPETHMAP_PIPE_NODEID_SLANT,
    // any node need to put upon this line
    eDPETHMAP_PIPE_NODE_SIZE,
} DepthMapPipeNodeID;

typedef std::bitset<eDPETHMAP_PIPE_NODE_SIZE> PipeNodeBitSet;

/**
 * @brief N3D preview phase
 */
enum N3D_PVPHASE_ENUM
{
    eN3D_PVPHASE_MAIN1_PADDING,
    eN3D_PVPHASE_GPU_WARPING,
    eN3D_PVPHASE_LEARNING
};

/**
  * @brief Data ID used in handleData inside the DepthMapPipe
 */
enum DepthMapDataID {
    ID_INVALID,
    ROOT_ENQUE,
    BAYER_ENQUE,
    // P2ANode
    P2A_TO_N3D_PADDING_MATRIX,
    P2A_TO_N3D_RECT2_FEO,
    P2A_TO_N3D_NOFEFM_RECT2,
    P2A_TO_N3D_CAP_RECT2,
    // P2ABayer
    P2A_TO_N3D_FEOFMO,
    P2A_TO_N3D_FEOFMO_NDD_L1,
    P2A_TO_DVP_MY_S,
    P2A_TO_DLDEPTH_MY_S,
    // N3DNode
    N3D_TO_WPE_RECT2_WARPMTX,
    N3D_TO_WPE_WARPMTX,
    N3D_TO_DVS_MVSV_MASK,
    // WPENode
    WPE_TO_DVS_WARP_IMG,
    WPE_TO_DLDEPTH_MV_SV,
    // DVS
    DVS_TO_DLDEPTH_MVSV_NOC,
    DVS_TO_DVP_NOC_CFM,
    DVS_TO_SLANT_NOC_CFM,
    // DVP
    DVP_TO_GF_DMW_N_DEPTH,
    DVP_TO_GF_DMW_N_DEPTH_NDD_L1,
    // DLDEPTH
    DLDEPTH_TO_SLANT_DEPTH,
    DLDEPTH_TO_GF_DEPTHMAP,
    // SLANT
    SLANT_TO_DVP_NOC_CFM,
    SLANT_TO_GF_DEPTH_CFM,
    // DepthMap output
    P2A_OUT_MV_F,
    P2A_OUT_FD,
    P2A_OUT_MV_F_CAP,
    GF_OUT_DMBG,
    GF_OUT_INTERNAL_DMBG,
    DEPTHMAP_META_OUT,
    // notify YUV done
    P2A_OUT_YUV_DONE,
    // notify HAL buffer is released
    DEPTHPIPE_BUF_RELEASED,
    // Depthmap output
    DVS_OUT_DEPTHMAP,
    DVS_OUT_INTERNAL_DEPTH,
    DVP_OUT_DEPTHMAP,
    DVP_OUT_INTERNAL_DEPTH,
    DLDEPTH_OUT_DEPTHMAP,
    DLDEPTH_OUT_INTERNAL_DEPTH,
    GF_OUT_DEPTHMAP,
    GF_OUT_INTERNAL_DEPTH,
    SLANT_OUT_DEPTHMAP,
    SLANT_OUT_INTERNAL_DEPTH,
    // eDEPTH_FLOW_TYPE_QUEUED_DEPTH specific
    P2A_NORMAL_FRAME_DONE,
    QUEUED_FLOW_DONE,
    // Notify error occur underlying the nodes
    ERROR_OCCUR_NOTIFY,
    // Notify the request's depthmap is not ready
    REQUEST_DEPTH_NOT_READY,
    // use to dump p1 raw buffers
    TO_DUMP_RAWS,
    // use to dump p1 yuv buffers
    TO_DUMP_YUVS,
    TO_DUMP_YUVS_NDD_L1,
    TO_DUMP_MAPPINGS,
    // use to dump buffers
    TO_DUMP_IMG3O,
    #ifdef GTEST
    // For UT
    UT_OUT_FE
    #endif
};

/**
  * @brief Data ID used in handleData inside the DepthMapPipe
 */
typedef enum eStereoP2Path
{
    eP2APATH_MAIN1,
    eP2APATH_MAIN2,
    eP2APATH_MAIN1_BAYER,
    eP2APATH_FE_MAIN1,
    eP2APATH_FE_MAIN2,
    eP2APATH_FM
} StereoP2Path;

typedef enum eINPUT_RAW_TYPE
{
    eRESIZE_RAW,
    eFULLSIZE_RAW
} INPUT_RAW_TYPE;

typedef enum eMETADATA_SOURCE
{
    USE_FLOW_DEFINED,
    USE_INTERNAL,
    USE_EXTERNAL
} METADATA_SOURCE;


MBOOL hasWPEHw();

/*******************************************************************************
* Structure Definition
********************************************************************************/

/*******************************************************************************
* Class Definition
********************************************************************************/
/**
 * @class DepthPipeLoggingSetup
 * @brief Control the logging enable of the depthmap pipe
 */

class DepthPipeLoggingSetup
{
public:
    static MBOOL mbProfileLog;
    static MUINT8 miDebugLog;
};

#undef VSDOF_LOGD
#undef VSDOF_PRFLOG
// logging macro
#define VSDOF_LOGD(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::miDebugLog >= 1) {MY_LOGD("%d: " fmt, __LINE__, ##arg);} } while(0)
#define VSDOF_LOGD2(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::miDebugLog >= 2) {MY_LOGD("%d: " fmt, __LINE__, ##arg);} } while(0)

#define VSDOF_MDEPTH_LOGD(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::miDebugLog) {CAM_ULOGD(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, \
                                                        "%d: " fmt, __LINE__, ##arg);} } while(0)
#define VSDOF_MDEPTH_LOGE(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::miDebugLog) {CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, \
                                                        "%d: " fmt, __LINE__, ##arg);} } while(0)
#define VSDOF_MDEPTH_LOGW(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::miDebugLog) {CAM_ULOGW(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, \
                                                        "%d: " fmt, __LINE__, ##arg);} } while(0)

#define VSDOF_PRFLOG(fmt, arg...) \
    do { if(DepthPipeLoggingSetup::mbProfileLog) {MY_LOGD("[VSDOF_Profile] %d: " fmt, __LINE__, ##arg);} } while(0)

// user load use MY_LOGE to logging
#ifdef USER_LOAD_PROFILE
    #define VSDOF_PRFTIME_LOG(fmt, arg...) \
        MY_LOGE("[VSDOF_Profile] %d: " fmt, __LINE__, ##arg);
    #define VSDOF_INIT_LOG(fmt, arg...) \
        MY_LOGE("[VSDOF_INIT] %d: " fmt, __LINE__, ##arg);
#else
    #define VSDOF_PRFTIME_LOG(fmt, arg...) \
        do { if(DepthPipeLoggingSetup::mbProfileLog) {MY_LOGD("[VSDOF_Profile] %d: " fmt, __LINE__, ##arg);} } while(0)
    #define VSDOF_INIT_LOG(fmt, arg...) \
        MY_LOGD("[VSDOF_INIT] %d: " fmt, __LINE__, ##arg);
#endif

/******************************************************************************
* Type Definition
********************************************************************************/
class DepthMapPipeNode;
typedef KeyedVector<DepthMapPipeNodeID, DepthMapPipeNode*> DepthMapPipeNodeMap;

const DepthMapBufferID INPUT_METADATA_PBID_LIST[] = {PBID_IN_APP_META, PBID_IN_HAL_META_MAIN1, PBID_IN_HAL_META_MAIN2, PBID_IN_P1_RETURN_META};
const DepthMapBufferID OUTPUT_METADATA_PBID_LIST[] = {PBID_OUT_APP_META, PBID_OUT_HAL_META};

}; // NSFeaturePipe_DepthMap
}; // NSCamFeature
}; // NSCam

#endif // _MTK_CAMERA_INCLUDE_STEREO_DEPTHMAP_FEATUREPIPE_COMMON_H_
