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
/**
 * @file DepthMapPipeUtils.h
 * @brief Utility functions and classes inside the DepthMapPipe
*/

#ifndef _MTK_CAMERA_INCLUDE_STEREO_DEPTHMAP_FEATUREPIPE_UTILS_H_
#define _MTK_CAMERA_INCLUDE_STEREO_DEPTHMAP_FEATUREPIPE_UTILS_H_


// Standard C header file

// Android system/core header file
#include <vector>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
// mtkcam custom header file

// mtkcam global header file
#include <DpDataType.h>
#include <mtkcam/drv/iopipe/Port.h>
#include <mtkcam/drv/iopipe/PostProc/IHalPostProcPipe.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IHalWpePipe.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam/def/common.h>
#include <mtkcam3/feature/stereo/pipe/IDepthMapPipe.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <isp_tuning.h>
// Module header file

// Local header file
#include "DepthMapPipe_Common.h"
#include "DepthMapEffectRequest.h"
// Log header file
#undef PIPE_CLASS_TAG
#define PIPE_MODULE_TAG "DepthMapPipe"
#define PIPE_CLASS_TAG "Utils"
#include <PipeLog.h>

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

using namespace android;
using namespace NSIoPipe;
using namespace NSCam::NSIoPipe::NSPostProc;
using std::vector;

typedef NSCam::NSIoPipe::NSWpe::WPEQParams WPEQParams;
/*******************************************************************************
* Enum Definition
********************************************************************************/
enum eCropGroup
{
    eCROP_CRZ=1,
    eCROP_WDMA=2,
    eCROP_WROT=3
};

enum eFrameIOType
{
    FRAME_INPUT,
    FRAME_OUTPUT
};


/*******************************************************************************
* Structure Definition
********************************************************************************/

/**
 * @struct eis_region
 * @brief Define the EIS output crop region
 **/
struct eis_region
{
    MUINT32 x_int      = 0;
    MUINT32 x_float    = 0;
    MUINT32 y_int      = 0;
    MUINT32 y_float    = 0;
    MSize   s          = MSize(0, 0);
    MINT32 gmvX        = 0;
    MINT32 gmvY        = 0;
    MINT32 confX       = 0;
    MINT32 confY       = 0;
    MINT32 maxGMV      = 0;
#if SUPPORT_EIS_MV
    MUINT32 x_mv_int   = 0;
    MUINT32 x_mv_float = 0;
    MUINT32 y_mv_int   = 0;
    MUINT32 y_mv_float = 0;
    MUINT32 is_from_zzr = 0;
#endif
};

/*******************************************************************************
* Function Definition
********************************************************************************/

/**
 * @brief Try to get metadata value
 * @param [in] pMetadata IMetadata instance
 * @param [in] tag the metadata tag to retrieve
 * @param [out] rVal the metadata value to be stored.
 * @return
 * - MTRUE indicates success.
 * - MFALSE indicates failure.
 */
template <typename T>
inline MBOOL
tryGetMetadata(
    IMetadata* pMetadata,
    MUINT32 const tag,
    T & rVal
)
{
    if( pMetadata == NULL ) {
        VSDOF_MDEPTH_LOGW("pMetadata == NULL");
        return MFALSE;
    }
    //
    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if( !entry.isEmpty() ) {
        rVal = entry.itemAt(0, Type2Type<T>());
        return MTRUE;
    }
    return MFALSE;
}

/**
 * @brief Try to set metadata value
 * @param [in] pMetadata IMetadata instance
 * @param [in] tag the metadata tag to configure
 * @param [in] rVal the metadata value to set
 * @return
 * - MTRUE indicates success.
 * - MFALSE indicates failure.
 */
template <typename T>
inline MVOID
trySetMetadata(
    IMetadata* pMetadata,
    MUINT32 const tag,
    T const& val
)
{
    if( pMetadata == NULL ) {
        VSDOF_MDEPTH_LOGW("pMetadata == NULL");
        return;
    }
    //
    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    pMetadata->update(tag, entry);
}

/**
 * @brief update the metadata entry
 * @param [in] pMetadata IMetadata instance
 * @param [in] tag the metadata tag to update
 * @param [in] rVal the metadata entry value
 * @return
 * - MTRUE indicates success.
 * - MFALSE indicates failure.
 */
template <typename T>
inline MVOID
updateEntry(
    IMetadata* pMetadata,
    MUINT32 const tag,
    T const& val
)
{
    if( pMetadata == NULL ) {
        VSDOF_MDEPTH_LOGW("pMetadata == NULL");
        return;
    }

    IMetadata::IEntry entry(tag);
    entry.push_back(val, Type2Type<T>());
    pMetadata->update(tag, entry);
}

/**
 * @brief Query the eis crop refion from the input HAL metadata
 */
MBOOL queryEisRegion(IMetadata* const inHal, eis_region& region);
/**
 * @brief Check EIS is on or not.
 */
MBOOL isEISOn(IMetadata* const inApp);
/**
 * @brief check 3DNR on/off
 */
MBOOL is3DNROn(IMetadata *appInMeta);
/*******************************************************************************
* Struct Definition
********************************************************************************/
/**
 * @brief Container class for the callback operations
 */
class DepthMapPipeNode;

struct EnqueCookieContainer
{
public:
    EnqueCookieContainer(sp<DepthMapEffectRequest> req, DepthMapPipeNode* pNode)
    : mRequest(req), mpNode(pNode) {}

    ~EnqueCookieContainer() {
        for(auto &data : mvData) {
            delete static_cast<WPEQParams*>(data);
        }
    }
public:
    sp<DepthMapEffectRequest> mRequest;
    DepthMapPipeNode* mpNode;
    vector<void*> mvData;
};
/*******************************************************************************
* Class Definition
********************************************************************************/

/**
 * @class QParamTemplateGenerator
 * @brief Store the location information of mvIn, mvOut, mvCropRsInfo for each frame
 **/
class QParamTemplateGenerator
{
private:
    QParamTemplateGenerator() = delete;
    MBOOL checkValid();
    MVOID debug(QParams& rQParam);
public:
    QParamTemplateGenerator(MUINT32 frameID, MUINT32 iSensorIdx, ENormalStreamTag streamTag);
    //
    QParamTemplateGenerator& addCrop(
                                eCropGroup groupID,
                                MPoint startLoc,
                                MSize cropSize,
                                MSize resizeDst,
                                bool isMDPCrop=false);
    //
    QParamTemplateGenerator& addInput(PortID portID);
    //
    QParamTemplateGenerator& addOutput(
                                PortID portID,
                                MINT32 transform=0,
                                EPortCapbility cap=EPortCapbility_None);
    //
    QParamTemplateGenerator& addModuleInfo(MUINT32 moduleTag, MVOID* moduleStruct);
    QParamTemplateGenerator& addExtraParam(EPostProcCmdIndex cmdIdx, MVOID* param);
    MBOOL generate(QParams& rQParamTmpl);

public:
    MUINT32 miFrameID;
    FrameParams mPass2EnqueFrame;
};

/**
 * @class QParamTemplateFiller
 * @brief Fill the corresponding input/output/tuning buffer and configure crop information
 **/
class QParamTemplateFiller
{

public:
    QParamTemplateFiller(
                    QParams& target,
                    DepthMapRequestPtr&,
                    DepthMapPipeNodeID nodeId);
    //
    QParamTemplateFiller& insertTuningBuf(MUINT frameID, MVOID* pTuningBuf);
    //
    QParamTemplateFiller& insertInputBuf(
                                MUINT frameID,
                                PortID portID,
                                IImageBuffer* pImggBuf);
    QParamTemplateFiller& insertInputBuf(
                                MUINT frameID,
                                PortID portID,
                                sp<EffectFrameInfo> pFrameInfo);
    //
    QParamTemplateFiller& insertOutputBuf(
                                MUINT frameID,
                                PortID portID,
                                IImageBuffer* pImggBuf);
    QParamTemplateFiller& insertOutputBuf(
                                MUINT frameID,
                                PortID portID,
                                sp<EffectFrameInfo> pFrameInfo);
    //
    QParamTemplateFiller& setOutputTransform(
                                MUINT frameID,
                                PortID portID,
                                MINT32 transform);
    //
    QParamTemplateFiller& setCrop(
                                MUINT frameID,
                                eCropGroup groupID,
                                MPoint startLoc,
                                MSize cropSize,
                                MSize resizeDst,
                                bool isMDPCrop = false);
    //
    QParamTemplateFiller& setCropResize(
                                MUINT frameID,
                                eCropGroup groupID,
                                MSize resizeDst);
    //
    QParamTemplateFiller& setIspProfile(
                                MUINT frameID,
                                NSIspTuning::EIspProfile_T profile);
    //
    QParamTemplateFiller& delInputPort(MUINT frameID, PortID portID);
    QParamTemplateFiller& delOutputPort(MUINT frameID, PortID portID, eCropGroup cropGID);
    QParamTemplateFiller& addExtraParam(MUINT frameID, EPostProcCmdIndex cmdIdx, MVOID* param);
    QParamTemplateFiller& addModuleInfo(MUINT frameID, MUINT32 moduleTag, MVOID* moduleStruct);
    //
    QParamTemplateFiller& addWPEQParam(
                                MUINT frameID,
                                IImageBuffer* pInputImg,
                                IImageBuffer* pWarpMatrix_X,
                                IImageBuffer* pWarpMatrix_Y,
                                NSIoPipe::NSWpe::WPE_MODE WPEMode,
                                EnqueCookieContainer*& rpWpeQParamHolder,
                                MBOOL const bUseHalfWPEConfigSize = MFALSE);
    /**
     * Validate the template filler status
     *
     * @return
     *  -  true if successful; otherwise false.
     **/
    MBOOL validate();
public:
    QParams& mTargetQParam;
    MBOOL mbSuccess;
};

/**
 * @brief remap int into iTransform
*/
MINT32 remapTransform(int rot);

/**
 * @brief QParams debug function
 */
MVOID debugQParams(const QParams& rInputQParam);

/**
 * @brief setup the dpIsppPram
 */
MBOOL
configurePQParam(
    DepthMapPipeNodeID,
    MINT32 iFrameNo,
    sp<DepthMapEffectRequest> pRequest,
    uint32_t sensorDevId,
    PQParam& rPqParam,
    MINT32 bMDPPortMask,   //bitwise check, WDMA port/WROT port
    MBOOL bDisablePQ = false
);

MBOOL
configureDpPQParam(
    DepthMapPipeNodeID srcNodeID,
    MINT32 iFrameNo,
    sp<DepthMapEffectRequest> pRequest,
    uint32_t sensorDevId,
    DpPqParam& rDpPqParam,
    MBOOL bDisablePQ = false
);

/**
 * @class TimeThreshold
 * @brief Profile the living time and logging when meet the threshold
 **/
#define TIME_THREHOLD(time, ...)\
    char buf[128];  \
    if ( snprintf(buf, 100, __VA_ARGS__ )>= 0){ \
        TimeThreshold thre(time, buf); }

class TimeThreshold
{
public:
    static const int LOG_LEN = 1024;
    TimeThreshold() = delete;
    /**
     * @param [in] threshold time in ms
     * @param [in] fmt logging string
     */
    TimeThreshold(
            int threshold,
            const char* fmt);

    ~TimeThreshold();

public:
    Timer mTimer;
    int miThreshold;
    char mLog[LOG_LEN]={};
};


}
}
}
#endif
