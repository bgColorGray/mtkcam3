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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#include <mtkcam/custom/ExifFactory.h>
#include <mtkcam/def/common.h>
#include <mtkcam/drv/def/Dip_Notify_datatype.h>
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/exif/DebugExifUtils.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/imgbuf/IGraphicImageBufferHeap.h>
#include <mtkcam3/feature/utils/p2/P2IOClassfier.h>
#include <mtkcam3/feature/lcenr/lcenr.h>
#include <DpDataType.h>

#include <mtkcam3/feature/utils/p2/P2Util.h>
#include <mtkcam3/feature/utils/p2/P2Trace.h>
#define ILOG_MODULE_TAG P2Util
#define ILOG_TRACE 0
#include <mtkcam3/feature/utils/log/ILogHeader.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_P2_PROC_COMMON);

using android::sp;
using NS3Av3::TuningParam;
using NS3Av3::IHal3A;
using NS3Av3::IHalISP;
using NS3Av3::MetaSet_T;
using NSCam::NSIoPipe::NSPostProc::INormalStream;
using NSCam::NSIoPipe::EPortCapbility;
using NSCam::NSIoPipe::EPortCapbility_None;
using NSCam::NSIoPipe::EPortCapbility_Disp;
using NSCam::NSIoPipe::EPortCapbility_Rcrd;
using NSCam::NSIoPipe::ModuleInfo;
using NSCam::NSIoPipe::EDIPInfoEnum;
using NSCam::NSIoPipe::NSMFB20::MSFConfig;

using NSImageio::NSIspio::EPortIndex;
using NSImageio::NSIspio::EPortIndex_IMGI;
using NSImageio::NSIspio::EPortIndex_IMGBI;
using NSImageio::NSIspio::EPortIndex_IMGCI;
using NSImageio::NSIspio::EPortIndex_VIPI;
using NSImageio::NSIspio::EPortIndex_DEPI;
using NSImageio::NSIspio::EPortIndex_LCEI;
using NSImageio::NSIspio::EPortIndex_DMGI;
using NSImageio::NSIspio::EPortIndex_BPCI;
using NSImageio::NSIspio::EPortIndex_LSCI;
using NSImageio::NSIspio::VirDIPPortIdx_YNR_FACEI;
using NSImageio::NSIspio::VirDIPPortIdx_YNR_LCEI;
using NSImageio::NSIspio::EPortIndex_IMG2O;
using NSImageio::NSIspio::EPortIndex_IMG3O;
using NSImageio::NSIspio::EPortIndex_WDMAO;
using NSImageio::NSIspio::EPortIndex_WROTO;
using NSImageio::NSIspio::EPortIndex_DCESO;
using NSImageio::NSIspio::EPortIndex_TIMGO;

using NSCam::NSIoPipe::PORT_IMGI;
using NSCam::NSIoPipe::PORT_IMGBI;
using NSCam::NSIoPipe::PORT_IMGCI;
using NSCam::NSIoPipe::PORT_VIPI;
using NSCam::NSIoPipe::PORT_DEPI;
using NSCam::NSIoPipe::PORT_LCEI;
using NSCam::NSIoPipe::PORT_DMGI;
using NSCam::NSIoPipe::PORT_BPCI;
using NSCam::NSIoPipe::PORT_LSCI;
using NSCam::NSIoPipe::PORT_YNR_FACEI;
using NSCam::NSIoPipe::PORT_YNR_LCEI;
using NSCam::NSIoPipe::PORT_IMG2O;
using NSCam::NSIoPipe::PORT_IMG3O;
using NSCam::NSIoPipe::PORT_WDMAO;
using NSCam::NSIoPipe::PORT_WROTO;
using NSCam::NSIoPipe::PORT_DCESO;
using NSCam::NSIoPipe::PORT_TIMGO;

using NSCam::NSIoPipe::EPIPE_FE_INFO_CMD;
using NSCam::NSIoPipe::EPIPE_FM_INFO_CMD;
using NSCam::NSIoPipe::EPIPE_MDP_PQPARAM_CMD;
using NSCam::NSIoPipe::EPIPE_IMG3O_CRSPINFO_CMD;
using NSCam::NSIoPipe::EPIPE_TIMGO_DUMP_SEL_CMD;
using NSCam::NSIoPipe::EPIPE_MSF_INFO_CMD;

using NSCam::NSIoPipe::FEInfo;
using NSCam::NSIoPipe::FMInfo;
using NSCam::NSIoPipe::CrspInfo;

namespace NSCam {
namespace Feature {
namespace P2Util {

/*******************************************
Static Internal function
*******************************************/

static MBOOL IS_LE(const MSize& src, const MSize& dst)
{
    return (src.w <= dst.w && src.h <= dst.h);
}

/*******************************************
Common function
*******************************************/

MVOID initBuffer(IImageBuffer *buffer)
{
    if(buffer)
    {
        for (size_t i = 0; i < buffer->getPlaneCount(); i++)
        {
            size_t size = buffer->getBufSizeInBytes(i);
            memset( (void*)buffer->getBufVA(i), 0, size);
        }
    }
}

MBOOL is4K2K(const MSize &size)
{
    const MINT32 UHD_VR_WIDTH = 3840;
    const MINT32 UHD_VR_HEIGHT = 2160;
    return (size.w >= UHD_VR_WIDTH && size.h >= UHD_VR_HEIGHT);
}

MCropRect getCropRect(const MRectF &rectF) {
    #define MAX_MDP_FRACTION_BIT (20) // MDP use 20bits
    MCropRect cropRect(rectF.p.toMPoint(), rectF.s.toMSize());
    cropRect.p_fractional.x = (rectF.p.x-cropRect.p_integral.x) * (1<<MAX_MDP_FRACTION_BIT);
    cropRect.p_fractional.y = (rectF.p.y-cropRect.p_integral.y) * (1<<MAX_MDP_FRACTION_BIT);
    cropRect.w_fractional = (rectF.s.w-cropRect.s.w) * (1<<MAX_MDP_FRACTION_BIT);
    cropRect.h_fractional = (rectF.s.h-cropRect.s.h) * (1<<MAX_MDP_FRACTION_BIT);
    return cropRect;
}

auto getDebugExif()
{
    static auto const sInst = MAKE_DebugExif();
    return sInst;
}

/*******************************************
P2IO function
*******************************************/
EPortCapbility toCapability(P2IO::TYPE type)
{
    switch(type)
    {
        case P2IO::TYPE_DISPLAY:    return EPortCapbility_Disp;
        case P2IO::TYPE_RECORD:     return EPortCapbility_Rcrd;
        default:                    return EPortCapbility_None;
    }
}

Output toOutput(const P2IO &io)
{
    Output out;
    out.mBuffer = io.mBuffer;
    out.mTransform = io.mTransform;
    out.mPortID.capbility = toCapability(io.mType);
    return out;
}

Output toOutput(const P2IO &io, MUINT32 index)
{
    Output out = toOutput(io);
    out.mPortID.index = index;
    return out;
}

MCropRect toMCropRect(const P2IO &io)
{
    MCropRect crop = getCropRect(io.mCropRect);
    applyDMAConstrain(crop, io.mDMAConstrain);
    return crop;
}

MCrpRsInfo toMCrpRsInfo(const P2IO &io)
{
    MCrpRsInfo info;
    info.mCropRect = toMCropRect(io);
    info.mResizeDst = io.mCropDstSize;
    return info;
}

MVOID applyDMAConstrain(MCropRect &crop, MUINT32 constrain)
{
    if( constrain & (DMAConstrain::NOSUBPIXEL|DMAConstrain::ALIGN2BYTE) )
    {
        crop.p_fractional.x = 0;
        crop.p_fractional.y = 0;
        crop.w_fractional = 0;
        crop.h_fractional = 0;
        if( constrain & DMAConstrain::ALIGN2BYTE )
        {
            crop.p_integral.x &= (~1);
            crop.p_integral.y &= (~1);
        }
    }
}

/*******************************************
Tuning function
*******************************************/

void* allocateRegBuffer(MBOOL zeroInit)
{
    MINT32 bCount = INormalStream::getRegTableSize();
    void *buffer = NULL;
    if(bCount > 0)
    {
        buffer = ::malloc(bCount);
        if( buffer && zeroInit )
        {
            memset(buffer, 0, bCount);
        }
    }
    return buffer;
}

MVOID releaseRegBuffer(void* &buffer)
{
    ::free(buffer);
    buffer = NULL;
}

TuningParam makeTuningParam(const ILog &log, const P2Pack &p2Pack, IHalISP *halISP, MetaSet_T &inMetaSet, MetaSet_T *pOutMetaSet, MBOOL resized, void *regBuffer, IImageBuffer *lcso, MINT32 dcesoMagic, IImageBuffer *dceso, MBOOL isSlave, void *syncTuningBuf, IImageBuffer *lcsho)
{
    (void)log;
    TRACE_S_FUNC_ENTER(log);
    TuningParam tuning;
    tuning.pRegBuf = regBuffer;
    tuning.pLcsBuf = lcso;
    tuning.pLceshoBuf = lcsho;
    tuning.pDcsBuf = (dcesoMagic >= 0) ? dceso : NULL;
    tuning.i4DcsMagicNo = dcesoMagic;
    tuning.bSlave = isSlave;
    tuning.pDualSynInfo = syncTuningBuf;

    if(dceso)
    {
        dceso->syncCache(eCACHECTRL_INVALID);
    }
    if(lcsho)
    {
        lcsho->syncCache(eCACHECTRL_INVALID);
    }

    trySet<MUINT8>(inMetaSet.halMeta, MTK_3A_PGN_ENABLE,
                   resized ? 0 : 1);
    P2_CAM_TRACE_BEGIN(TRACE_DEFAULT, "P2Util:Tuning");
    if( halISP && regBuffer )
    {
        MINT32 ret3A = halISP->setP2Isp(0, inMetaSet, &tuning, pOutMetaSet);
        if( ret3A < 0 )
        {
            MY_S_LOGW(log, "hal3A->setIsp failed, memset regBuffer to 0");
            if( tuning.pRegBuf )
            {
                memset(tuning.pRegBuf, 0, INormalStream::getRegTableSize());
            }
        }
        if( pOutMetaSet )
        {
            updateExtraMeta(p2Pack, pOutMetaSet->halMeta);
            updateDebugExif(p2Pack, inMetaSet.halMeta, pOutMetaSet->halMeta);
        }
    }
    else
    {
        MY_S_LOGE(log, "cannot run setIsp: hal3A=%p reg=%p", halISP, regBuffer);
    }
    P2_CAM_TRACE_END(TRACE_DEFAULT);

    TRACE_S_FUNC_EXIT(log);
    return tuning;
}

/*******************************************
Metadata function
*******************************************/

MVOID updateExtraMeta(const P2Pack &p2Pack, IMetadata &outHal)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_S_FUNC_ENTER(p2Pack.mLog);
    trySet<MINT32>(outHal, MTK_PIPELINE_FRAME_NUMBER, p2Pack.getFrameData().mMWFrameNo);
    trySet<MINT32>(outHal, MTK_PIPELINE_REQUEST_NUMBER, p2Pack.getFrameData().mMWFrameRequestNo);
    TRACE_S_FUNC_EXIT(p2Pack.mLog);
}

MVOID updateDebugExif(const P2Pack &p2Pack, const IMetadata &inHal, IMetadata &outHal)
{
    (void)p2Pack;
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_S_FUNC_ENTER(p2Pack.mLog);
    MUINT8 needExif = 0;
    if( tryGet<MUINT8>(inHal, MTK_HAL_REQUEST_REQUIRE_EXIF, needExif) &&
        needExif )
    {
        MINT32 vhdrMode = SENSOR_VHDR_MODE_NONE;
        if( tryGet<MINT32>(inHal, MTK_P1NODE_SENSOR_VHDR_MODE, vhdrMode) &&
            vhdrMode != SENSOR_VHDR_MODE_NONE )
        {
            std::map<MUINT32, MUINT32> debugInfoList;
            debugInfoList[getDebugExif()->getTagId_MF_TAG_IMAGE_HDR()] = 1;

            IMetadata exifMeta;
            tryGet<IMetadata>(outHal, MTK_3A_EXIF_METADATA, exifMeta);
            if( DebugExifUtils::setDebugExif(
                    DebugExifUtils::DebugExifType::DEBUG_EXIF_MF,
                    static_cast<MUINT32>(MTK_MF_EXIF_DBGINFO_MF_KEY),
                    static_cast<MUINT32>(MTK_MF_EXIF_DBGINFO_MF_DATA),
                    debugInfoList, &exifMeta) != NULL )
            {
                trySet<IMetadata>(outHal, MTK_3A_EXIF_METADATA, exifMeta);
            }
        }
    }
    TRACE_S_FUNC_EXIT(p2Pack.mLog);
}

MBOOL updateCropRegion(IMetadata &outHal, const MRect &rect)
{
    // TODO: set output crop w/ margin setting
    MBOOL ret = MFALSE;
    ret = trySet<MRect>(outHal, MTK_SCALER_CROP_REGION, rect);
    return ret;
}

/*******************************************
DIPParams util function
*******************************************/

EPortCapbility toCapability(MUINT32 usage)
{
    EPortCapbility cap = EPortCapbility_None;
    if( usage & (GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_TEXTURE) )
    {
        cap = EPortCapbility_Disp;
    }
    else if( usage & GRALLOC_USAGE_HW_VIDEO_ENCODER )
    {
        cap = EPortCapbility_Rcrd;
    }
    return cap;
}

const char* toName(EPortIndex index)
{
    switch( index )
    {
    case EPortIndex_IMGI:           return "imgi";
    case EPortIndex_IMGBI:          return "imgbi";
    case EPortIndex_IMGCI:          return "imgci";
    case EPortIndex_VIPI:           return "vipi";
    case EPortIndex_DEPI:           return "depi";
    case EPortIndex_LCEI:           return "lcei";
    case EPortIndex_DMGI:           return "dmgi";
    case EPortIndex_BPCI:           return "bpci";
    case EPortIndex_LSCI:           return "lsci";
    case VirDIPPortIdx_YNR_FACEI:   return "ynr_facei";
    case VirDIPPortIdx_YNR_LCEI:    return "ynr_lcei";
    case EPortIndex_IMG2O:          return "img2o";
    case EPortIndex_IMG3O:          return "img3o";
    case EPortIndex_WDMAO:          return "wdmao";
    case EPortIndex_WROTO:          return "wroto";
    case EPortIndex_DCESO:          return "dceso";
    case EPortIndex_TIMGO:          return "timgo";
    default:                        return "unknown";
    };
    return NULL;
}

const char* toName(MUINT32 index)
{
    return toName((EPortIndex)index);
}

const char* toName(const NSCam::NSIoPipe::PortID &port)
{
    return toName((EPortIndex)port.index);
}

const char* toName(const Input &input)
{
    return toName((EPortIndex)input.mPortID.index);
}

const char* toName(const Output &output)
{
    return toName((EPortIndex)output.mPortID.index);
}

MBOOL is(const PortID &port, EPortIndex index)
{
    return port.index == index;
}

MBOOL is(const Input &input, EPortIndex index)
{
    return input.mPortID.index == index;
}

MBOOL is(const Output &output, EPortIndex index)
{
    return output.mPortID.index == index;
}

MBOOL is(const PortID &port, const PortID &rhs)
{
    return port.index == rhs.index;
}

MBOOL is(const Input &input, const PortID &rhs)
{
    return input.mPortID.index == rhs.index;
}

MBOOL is(const Output &output, const PortID &rhs)
{
    return output.mPortID.index == rhs.index;
}

MBOOL findInput(const DIPFrameParams &frame, const PortID &portID, Input &input)
{
    MBOOL ret = MFALSE;
    for(const Input &i : frame.mvIn)
    {
        if(is(i, portID))
        {
            input = i;
            ret = MTRUE;
            break;
        }
    }
    return ret;
}

MBOOL findOutput(const DIPFrameParams &frame, const PortID &portID, Output &output)
{
    MBOOL ret = MFALSE;
    for(const Output &o : frame.mvOut)
    {
        if(is(o, portID))
        {
            output = o;
            ret = MTRUE;
            break;
        }
    }
    return ret;
}

IImageBuffer* findInputBuffer(const DIPFrameParams &frame, const PortID &portID)
{
    IImageBuffer *buffer = NULL;
    Input input;
    if(findInput(frame, portID, input))
    {
        buffer = input.mBuffer;
    }

    return buffer;
}

MBOOL isGraphicBuffer(const IImageBuffer *imageBuffer)
{
    IImageBufferHeap *imageBufferHeap = (imageBuffer == NULL) ? NULL : imageBuffer->getImageBufferHeap();
    return ((imageBufferHeap != NULL) && (imageBufferHeap->getHWBuffer() != NULL));
}

MVOID printDIPParams(const ILog &log, unsigned i, const Input &input)
{
    unsigned index = input.mPortID.index;
    if( input.mBuffer != NULL)
    {
        MSize size = input.mBuffer->getImgSize();
        MINT fmt = input.mBuffer->getImgFormat();
        MINT32 transform = input.mTransform;
        MINTPTR pa = input.mBuffer->getBufPA(0);
        MUINTPTR secHandle = input.mSecHandle;
        S_LOGD(log, "[PrintQ]: mvIn[%d] idx=%d(%s) size=(%dx%d) fmt=0x%08x transform=%d pa=0x%016" PRIxPTR " sec=0x%016" PRIxPTR, i, index, toName(index), size.w, size.h, fmt, transform, pa, secHandle);
    }
    else
    {
        S_LOGD(log, "[PrintQ]: mvIn[%d] idx=%d(%s) buffer is NULL", i, index, toName(index));
    }
}

MVOID printBuffer(const ILog &log, unsigned i, const IImageBuffer *buffer, const char *prefix)
{
    if( buffer != NULL)
    {
        MSize size = buffer->getImgSize();
        MINT fmt = buffer->getImgFormat();
        MINTPTR pa = buffer->getBufPA(0);
        MUINT32 strideB = buffer->getBufStridesInBytes(0);
        S_LOGD(log, "[PrintQ]: %s : [%d] size=(%dx%d) stride=%d fmt=0x%08x pa=0x%016" PRIxPTR , prefix, i, size.w, size.h, strideB, fmt, pa);
    }
    else
    {
        S_LOGD(log, "[PrintQ]: %s : [%d] NULL", prefix, i);
    }
}

MVOID printBuffers(const ILog &log, std::vector<IImageBuffer*> vBuf, const char *prefix)
{
    for( unsigned i = 0, n = vBuf.size(); i < n; ++i )
    {
        printBuffer(log, i, vBuf[i], prefix);
    }
}

MVOID printDIPParams(const ILog &log, unsigned i, const Output &output)
{
    unsigned index = output.mPortID.index;
    if( output.mBuffer != NULL)
    {
        MSize size = output.mBuffer->getImgSize();
        MBOOL isGraphic = isGraphicBuffer(output.mBuffer);
        MINT fmt = output.mBuffer->getImgFormat();
        MUINT32 cap = output.mPortID.capbility;
        MINT32 transform = output.mTransform;
        MINTPTR pa = output.mBuffer->getBufPA(0);
        S_LOGD(log, "[PrintQ]: mvOut[%d] idx=%d(%s) size=(%dx%d) fmt=0x%08x, cap=%02x, isGraphic=%d transform=%d pa=0x%016" PRIxPTR , i, index, toName(index), size.w, size.h, fmt, cap, isGraphic, transform, pa);
    }
    else
    {
        S_LOGD(log, "[PrintQ]: mvOut[%d] idx=%d(%s) buffer is NULL", i, index, toName(index));
    }
}

MVOID printDIPParams(const ILog &log, unsigned i, const MCrpRsInfo &crop)
{
    S_LOGD(log, "[PrintQ]: crop[%d] groupID=%d frameGroup=%d i(%dx%d) f(%dx%d) s(%dx%d) r(%dx%d)", i, crop.mGroupID, crop.mFrameGroup, crop.mCropRect.p_integral.x, crop.mCropRect.p_integral.y,
        crop.mCropRect.p_fractional.x, crop.mCropRect.p_fractional.y, crop.mCropRect.s.w, crop.mCropRect.s.h, crop.mResizeDst.w, crop.mResizeDst.h);

}

MVOID printSRZ(const ILog &log, unsigned i, unsigned srzNum, const _SRZ_SIZE_INFO_ *sizeInfo)
{
    if(sizeInfo != NULL)
    {
        S_LOGD(log, "[PrintQ]: moduleInfo[%d] SRZ%d in(%dx%d) crop(%lux%lu) crop_start=(%d.%u, %d.%u) out(%dx%d)", i, srzNum, sizeInfo->in_w, sizeInfo->in_h, sizeInfo->crop_w, sizeInfo->crop_h,
            sizeInfo->crop_x, sizeInfo->crop_floatX, sizeInfo->crop_y, sizeInfo->crop_floatY, sizeInfo->out_w, sizeInfo->out_h);
    }
    else
    {
        S_LOGD(log, "[PrintQ]: moduleInfo[%d] SRZ%d moduleStruct is NULL", i, srzNum);
    }
}

MVOID printDIPParams(const ILog &log, unsigned i, const ModuleInfo &info)
{
    _SRZ_SIZE_INFO_ *sizeInfo = (_SRZ_SIZE_INFO_*)info.moduleStruct;
    switch(info.moduleTag)
    {
        case EDipModule_SRZ1:
            printSRZ(log, i, 1, sizeInfo);
            break;
        case EDipModule_SRZ3:
            printSRZ(log, i, 3, sizeInfo);
            break;
        case EDipModule_SRZ4:
            printSRZ(log, i, 4, sizeInfo);
            break;
        default:
            S_LOGD(log, "[PrintQ]: moduleInfo[%d] unknow tag(%d)", i, info.moduleTag);
            break;
    };
}

MVOID printFeInfo(const ILog &log, unsigned i, FEInfo *feInfo)
{
    if(feInfo != NULL)
    {
        S_LOGD(log, "[PrintQ]: extra[%d] FE_CMD DSCR_SBIT=%d TH_C=%d TH_G=%d FLT_EN=%d PARAM=%d MODE=%d YIDX=%d XIDX=%d START_X=%d START_Y=%d IN_HT=%d IN_WD=%d", i,
            feInfo->mFEDSCR_SBIT, feInfo->mFETH_C, feInfo->mFETH_G, feInfo->mFEFLT_EN, feInfo->mFEPARAM, feInfo->mFEMODE, feInfo->mFEYIDX, feInfo->mFEXIDX, feInfo->mFESTART_X, feInfo->mFESTART_Y, feInfo->mFEIN_HT, feInfo->mFEIN_WD);
    }
}

MVOID printFmInfo(const ILog &log, unsigned i, FMInfo *fmInfo)
{
    if(fmInfo != NULL)
    {
        S_LOGD(log, "[PrintQ]: extra[%d] FM_CMD HEIGHT=%d WIDTH=%d SR_TYPE=%d OFFSET_X=%d OFFSET_Y=%d RES_TH=%d SAD_TH=%d MIN_RATIO=%d", i,
            fmInfo->mFMHEIGHT, fmInfo->mFMWIDTH, fmInfo->mFMSR_TYPE, fmInfo->mFMOFFSET_X, fmInfo->mFMOFFSET_Y, fmInfo->mFMRES_TH, fmInfo->mFMSAD_TH, fmInfo->mFMMIN_RATIO);
    }
}

MVOID printDpPqParam(const ILog &log, unsigned i, const DpPqParam *pq, const char *name)
{
    if( pq )
    {
        const DpIspParam *isp = &(pq->u.isp);
        const ClearZoomParam *cz = &(pq->u.isp.clearZoomParam);
        const DpDREParam *dre = &(pq->u.isp.dpDREParam);
        const DpHFGParam *hfg = &(pq->u.isp.dpHFGParam);
        S_LOGD(log,
            "[PrintQ]: extra[%d] PQ_%s_CMD scenario=%d enable=%d"
            " isp.timestamp=%d isp.frameNo=%d isp.requestNo=%d"
            " isp.lensID=%d isp.iso=%d isp.LV=%d isp.p_faceInfor=%p isp.pLCE=%p isp.LCESize=%d"
            " cz.captureShot=%d cz.p_customSetting=%p"
            " dre.cmd=%d dre.user=%llu dre.buf=%p dre.custP=%p dre.custId=%d"
            " hfg.set=%p/%p hfg.iso=%d/%d hfg.ispTun=%p"
            " recordType=%d gHandle=%p",
            i, name, pq->scenario, pq->enable,
            isp->timestamp, isp->frameNo, isp->requestNo,
            isp->lensId, isp->iso, isp->LV, isp->p_faceInfor, isp->LCE, isp->LCE_Size,
            cz->captureShot, cz->p_customSetting,
            dre->cmd, (unsigned long long)dre->userId, dre->buffer,
            dre->p_customSetting, dre->customIndex,
            hfg->p_lowerSetting, hfg->p_upperSetting,
            hfg->lowerISO, hfg->upperISO, hfg->p_slkParam,
            isp->recordType, isp->grallocExtraHandle);
    }
}

MVOID printDIPParamsInfo(const ILog &log, unsigned i, PQParam *pqParam)
{
    if( pqParam )
    {
        S_LOGD(log, "[PrintQ]: extra[%d] PQ_CMD pqParam=%p pqParam.WDMA=%p pqParam.WROT=%p", i, pqParam, pqParam->WDMAPQParam, pqParam->WROTPQParam);
        printDpPqParam(log, i, (DpPqParam*)pqParam->WDMAPQParam, "WDMA");
        printDpPqParam(log, i, (DpPqParam*)pqParam->WROTPQParam, "WROT");
    }
}

MVOID printMSFInfo(const ILog &log, unsigned i, MSFConfig *cfg)
{
    if(cfg != NULL)
    {
        S_LOGD(log, "[PrintQ]: extra[%d] MSF_CMD scenario=%d fId=%u, fTotal=%u, scaleId=%u, scaleTotal=%u, msfTun=%zu, msfSram=%zu",
               i, cfg->msf_scenario, cfg->frame_Idx, cfg->frame_Total, cfg->scale_Idx, cfg->scale_Total, cfg->msf_tuningBuf.size(), cfg->msf_sramTable.size());
        printBuffers(log, cfg->msf_baseFrame, "base_frame");
        printBuffers(log, cfg->msf_refFrame, "ref_frame");
        printBuffers(log, cfg->msf_weightingMap_in, "weight");
        printBuffers(log, cfg->msf_weightingMap_ds, "dsWeight(0:Out,1:In)");
        printBuffer(log, 0, cfg->msf_dsFrame, "ds_frame");
        printBuffer(log, 0, cfg->msf_confidenceMap, "confi");
        printBuffer(log, 0, cfg->msf_imageDifference, "idi");
    }
}

MVOID printCrspInfo(const ILog &log, unsigned i, CrspInfo *crspInfo)
{
    if(crspInfo != NULL)
    {
        S_LOGD(log, "[PrintQ]: extra[%d] CRSPINFO_CMD OFFSET_X=%d OFFSET_Y=%d WIDTH=%d HEIGHT=%d", i, crspInfo->m_CrspInfo.p_integral.x, crspInfo->m_CrspInfo.p_integral.y, crspInfo->m_CrspInfo.s.w, crspInfo->m_CrspInfo.s.h);
    }
}

MVOID printTimgoInfo(const ILog &log, unsigned i, MUINT32 *timgoInfo)
{
    S_LOGD(log, "[PrintQ]: extra[%d] TIMGO_DUMP_SEL_CMD val=%d addr=%p",
                i, timgoInfo ? *timgoInfo : 0, timgoInfo);
}

MVOID printDIPParams(const ILog &log, unsigned i, const ExtraParam &ext)
{
    switch(ext.CmdIdx)
    {
        case EPIPE_FE_INFO_CMD:
            printFeInfo(log, i, (FEInfo*)ext.moduleStruct); break;
        case EPIPE_FM_INFO_CMD:
            printFmInfo(log, i, (FMInfo*)ext.moduleStruct); break;
        case EPIPE_MDP_PQPARAM_CMD:
            printDIPParamsInfo(log, i, (PQParam*)ext.moduleStruct); break;
        case EPIPE_IMG3O_CRSPINFO_CMD:
            printCrspInfo(log, i, (CrspInfo*)ext.moduleStruct); break;
        case EPIPE_TIMGO_DUMP_SEL_CMD:
            printTimgoInfo(log, i, (MUINT32*)ext.moduleStruct); break;
        case EPIPE_MSF_INFO_CMD:
            printMSFInfo(log, i, (MSFConfig*)ext.moduleStruct); break;
        default:
            S_LOGD(log, "[PrintQ]: unknown ext: idx=%d", ext.CmdIdx); break;
    };
}

MVOID printDIPParams(const ILog &log, const DIPParams &params)
{
    for( unsigned f = 0, fCount = params.mvDIPFrameParams.size(); f < fCount; ++f )
    {
        const DIPFrameParams &frame = params.mvDIPFrameParams[f];
        struct timeval curr;
        gettimeofday(&curr, NULL);
        S_LOGD(log, "[PrintQ]: DIPParams frame(%d/%d) FrameNo=%u RequestNo=%u Timestamp=%u UniqueKey=%d StreamTag=%d SensorIdx=%d SecureFra=%d", f, fCount, frame.FrameNo, frame.RequestNo, frame.Timestamp, frame.UniqueKey, frame.mStreamTag, frame.mSensorIdx, frame.mSecureFra);
        S_LOGD(log, "[PrintQ]: DIPParams frame(%d/%d) mTuningData=%p CurrentTime=%d.%03d EndTime=%d.%03d fps=%d Width=%d", f, fCount, frame.mTuningData,(MUINT32)curr.tv_sec%1000, (MUINT32)curr.tv_usec/1000, (MUINT32)frame.ExpectedEndTime.tv_sec%1000, (MUINT32)frame.ExpectedEndTime.tv_usec/1000, frame.Fps, frame.Resolution_Width);

        for( unsigned i = 0, n = frame.mvIn.size(); i < n; ++i )
        {
            printDIPParams(log, i, frame.mvIn[i]);
        }
        for( unsigned i = 0, n = frame.mvOut.size(); i < n; ++i )
        {
            printDIPParams(log, i, frame.mvOut[i]);
        }
        for( unsigned i = 0, n = frame.mvCropRsInfo.size(); i < n; ++i )
        {
            printDIPParams(log, i, frame.mvCropRsInfo[i]);
        }
        for( unsigned i = 0, n = frame.mvModuleData.size(); i < n; ++i )
        {
            printDIPParams(log, i, frame.mvModuleData[i]);
        }
        for( unsigned i = 0, n = frame.mvExtraParam.size(); i < n; ++i )
        {
            printDIPParams(log, i, frame.mvExtraParam[i]);
        }
    }
}

MVOID printTuningParam(const ILog &log, const TuningParam &tuning, unsigned index)
{
    S_LOGD(log, "[PrintQ] tun[%d] reg=%p msfTun=%p msfTab=%p lcs=%p lsc2=%p bpc2=%p dcs=%d/%d/%p dual=%d/%p faceAlpha=%p dre=%p",
                index, tuning.pRegBuf, tuning.pMfbBuf, tuning.pMsfTbl, tuning.pLcsBuf,
                tuning.pLsc2Buf, tuning.pBpc2Buf,
                tuning.bDCES_Enalbe, tuning.i4DcsMagicNo, tuning.pDcsBuf,
                tuning.bSlave, tuning.pDualSynInfo,
                tuning.pFaceAlphaBuf, tuning.pLce4CALTM);
}

MVOID printTuningParam(const TuningParam &tuning)
{
    printTuningParam(ILog(), tuning);
}

MVOID printPQCtrl(const char *name, const PQCtrl *pqCtrl, const char* caller)
{
    if( pqCtrl )
    {
        MY_LOGD("%s: name(%s) sId(%d) dre:2nd(%d)tun(%p)size(%d)",
                name, caller ? caller : pqCtrl->getDebugName(), pqCtrl->getSensorID(),
                pqCtrl->is2ndDRE(), pqCtrl->getDreTuning(), pqCtrl->getDreTuningSize());
    }
    else
    {
        MY_LOGD("%s: missing pqCtrl=NULL", name);
    }
}

MVOID printPQCtrl(const char *name, const IPQCtrl_const &pqCtrl, const char* caller)
{
    printPQCtrl(name, pqCtrl.get(), caller);
}

MVOID printDpPqParam(const char *name, const DpPqParam *pq)
{
    printDpPqParam(ILog(), 0, pq, name);
}

MVOID push_in(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer)
{
    Input input;
    input.mPortID = portID,
    input.mPortID.group = 0;
    input.mBuffer = buffer;
    frame.mvIn.push_back(input);
}


MVOID push_msfIn(DIPFrameParams &frame, const P2IOPack &ioPack, const TuningParam &tuning, MSFConfig *msfConfig)
{
    msfConfig->msf_dsFrame = ioPack.mMSFDSI;
    msfConfig->msf_baseFrame.push_back(ioPack.mIMGI.mBuffer);

    if( ioPack.mMSFREFI )
    {
        msfConfig->msf_tuningBuf.push_back( (unsigned int*)tuning.pMfbBuf );
        msfConfig->msf_sramTable.push_back( (unsigned int*)tuning.pMsfTbl );
        msfConfig->msf_imageDifference = ioPack.mMSFIDI;
        msfConfig->msf_confidenceMap = ioPack.mMSFCONFI;
        msfConfig->msf_refFrame.push_back(ioPack.mMSFREFI);
        msfConfig->msf_weightingMap_in.push_back(ioPack.mMSFWEIGHTI);
        msfConfig->msf_weightingMap_ds.push_back(ioPack.mMSFDSWO);
        msfConfig->msf_weightingMap_ds.push_back(ioPack.mMSFDSWI);
    }

    ExtraParam extra;
    extra.CmdIdx = NSIoPipe::EPIPE_MSF_INFO_CMD;
    extra.moduleStruct = (void*)msfConfig;
    frame.mvExtraParam.push_back(extra);
}

MVOID push_in(DIPFrameParams &frame, const PortID &portID, const P2IO &in)
{
    push_in(frame, portID, in.mBuffer);
}

MVOID push_out(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer)
{
    push_out(frame, portID, buffer, P2IO::TYPE_UNKNOWN, 0);
}

MVOID push_out(DIPFrameParams &frame, const PortID &portID, IImageBuffer *buffer, P2IO::TYPE type, MINT32 transform)
{
    Output output;
    output.mPortID = portID;
    output.mPortID.group = 0;
    output.mPortID.capbility = toCapability(type);
    output.mTransform = transform;
    output.mBuffer = buffer;
    frame.mvOut.push_back(output);
}

MVOID push_out(DIPFrameParams &frame, const PortID &portID, const P2IO &out)
{
    push_out(frame, portID, out.mBuffer, out.mType, out.mTransform);
}

MVOID push_crop(DIPFrameParams &frame, MUINT32 cropID, const MCropRect &crop, const MSize &size)
{
    MCrpRsInfo cropInfo;
    cropInfo.mGroupID = cropID;
    cropInfo.mCropRect = crop;
    cropInfo.mResizeDst = size;
    frame.mvCropRsInfo.push_back(cropInfo);
}

MVOID push_crop(DIPFrameParams &frame, MUINT32 cropID, const MRectF &crop, const MSize &size, const MUINT32 dmaConstrain)
{
    MCrpRsInfo cropInfo;
    cropInfo.mGroupID = cropID;
    cropInfo.mCropRect = getCropRect(crop);

    if( (dmaConstrain & DMAConstrain::NOSUBPIXEL) ||
        (dmaConstrain & DMAConstrain::ALIGN2BYTE) )
    {
        cropInfo.mCropRect.p_fractional.x = 0;
        cropInfo.mCropRect.p_fractional.y = 0;
        cropInfo.mCropRect.w_fractional = 0;
        cropInfo.mCropRect.h_fractional = 0;
        if( dmaConstrain & DMAConstrain::ALIGN2BYTE )
        {
            cropInfo.mCropRect.p_integral.x &= (~1);
            cropInfo.mCropRect.p_integral.y &= (~1);
        }
    }
    cropInfo.mResizeDst = size;
    frame.mvCropRsInfo.push_back(cropInfo);
}

/*******************************************
DIPParams function
*******************************************/

MBOOL push_srz4_LCENR(DIPFrameParams &frame, _SRZ_SIZE_INFO_ *srz4, const P2Pack &p2Pack, MBOOL resized, const MSize &imgiSize, const MSize &lcsoSize)
{
    if( srz4 )
    {
        LCENR_IN_PARAMS in;
        LCENR_OUT_PARAMS out;
        const P2SensorData &data = p2Pack.getSensorData();

        in.resized = resized;
        in.p2_in = imgiSize;
        in.rrz_in = data.mP1BinSize;
        in.rrz_crop_in = data.mP1BinCrop;
        in.rrz_out = data.mP1OutSize;
        in.lce_full = lcsoSize;
        calculateLCENRConfig(in, out);

        *srz4 = out.srz4Param;

        ModuleInfo info;
        info.moduleTag = EDipModule_SRZ4;
        info.frameGroup = 0;
        info.moduleStruct = reinterpret_cast<MVOID*>(srz4);
        frame.mvModuleData.push_back(info);
    }
    return (srz4 != NULL);
}

MBOOL push_srz4_MOTION_NR(DIPFrameParams &frame, _SRZ_SIZE_INFO_ *srz4, const MSize &p2InSize, const MSize &lceiSize)
{
    if( srz4 )
    {
        MOTIONNR_IN_PARAMS in;
        MOTIONNR_OUT_PARAMS out;
        in.p2_in = p2InSize;
        in.motion_map = lceiSize;

        calculateMOTIONNRConfig(in, out);

        *srz4 = out.srz4Param;

        ModuleInfo info;
        info.moduleTag = EDipModule_SRZ4;
        info.frameGroup = 0;
        info.moduleStruct = reinterpret_cast<MVOID*>(srz4);
        frame.mvModuleData.push_back(info);
    }
    return (srz4 != NULL);
}

MBOOL push_srz3_FACENR(DIPFrameParams &frame, _SRZ_SIZE_INFO_ *srz3, const MSize &imgiSize, const MSize &faceiSize)
{
    if( srz3 )
    {
        FACENR_IN_PARAMS in;
        FACENR_OUT_PARAMS out;

        in.p2_in = imgiSize;
        in.face_map = faceiSize;
        calculateFACENRConfig(in, out);

        *srz3 = out.srz3Param;

        ModuleInfo info;
        info.moduleTag = EDipModule_SRZ3;
        info.frameGroup = 0;
        info.moduleStruct = reinterpret_cast<MVOID*>(srz3);
        frame.mvModuleData.push_back(info);
    }
    return (srz3 != NULL);
}

eP2_PQ_PATH toP2PQPath(MUINT32 type /* P2IO::TYPE */)
{
    eP2_PQ_PATH path = P2_PQ_PATH_OTHER;
    switch( type )
    {
    case P2IO::TYPE_DISPLAY:    path = P2_PQ_PATH_DISPLAY;  break;
    case P2IO::TYPE_RECORD:     path = P2_PQ_PATH_RECORD;   break;
    case P2IO::TYPE_VSS:        path = P2_PQ_PATH_VSS;      break;
    default:                    path = P2_PQ_PATH_OTHER;    break;
    }
    return path;
}

ISP_RECORD_SCENARIO_ENUM transHdr10SpecToScenario(MINT32 hdr10Spec)
{
    switch( hdr10Spec )
    {
    case 1:
        return RECORD_HDR10;
    case 2:
        return RECORD_HLG;
    case 0: // go through
    default:
        break;
    }

    return RECORD_HDR10_PLUS;
}

DpPqParam makeDpPqParam(const P2Pack &p2Pack, MUINT32 sensorID)
{
    TRACE_FUNC_ENTER();
    DpPqParam pq;
    DpIspParam &isp = pq.u.isp;

    const P2ConfigInfo &configInfo = p2Pack.getConfigInfo();
    const P2FrameData &frameData = p2Pack.getFrameData();
    const P2SensorData &sensorData = p2Pack.getSensorData(sensorID);
    const P2SensorInfo &sensorInfo = p2Pack.getSensorInfo(sensorID);
    const P2PlatInfo *plat = sensorInfo.mPlatInfo;

    pq.scenario = MEDIA_ISP_PREVIEW;
    pq.enable = 0;

    isp.iso = sensorData.mISO;
    isp.LV = sensorData.mLV;
    isp.timestamp = sensorData.mMWUniqueKey;
    isp.frameNo = frameData.mMWFrameNo;
    isp.requestNo = frameData.mMWFrameRequestNo;
    isp.lensId = sensorData.mSensorID;
    isp.enableDump = false;
    isp.userString[0] = '\0';
    isp.recordType = RECORD_NONLINEAR;

    if( configInfo.mSupportPQ )
    {
        if( configInfo.mSupportClearZoom && plat->supportClearZoom() )
        {
            ClearZoomParam &cz = isp.clearZoomParam;
            cz.captureShot = CAPTURE_SINGLE;
            P2PlatInfo::NVRamData nvramData = plat->queryNVRamData(P2PlatInfo::FID_CLEARZOOM, sensorData.mMagic3A, sensorData.mIspProfile);
            cz.p_customSetting = nvramData.mLowIsoData;
        }
        if( configInfo.mSupportHFG && plat->supportHFG() )
        {
            DpHFGParam &hfg = isp.dpHFGParam;
            P2PlatInfo::NVRamData nvramData = plat->queryNVRamData(P2PlatInfo::FID_HFG, sensorData.mMagic3A, sensorData.mIspProfile);
            hfg.p_lowerSetting = nvramData.mLowIsoData;
            hfg.p_upperSetting = nvramData.mLowIsoData;
            hfg.lowerISO = isp.iso;
            hfg.upperISO = isp.iso;
            // hfg.p_upperSetting = nvramData.mHighIsoData;
            // hfg.lowerISO = nvramData.mLowerIso;
            // hfg.upperISO = nvramData.mHighIso;
        }
        if( configInfo.mSupportDRE && plat->supportDRE() )
        {
            DpDREParam &dre = isp.dpDREParam;
            dre.cmd = DpDREParam::Cmd::Initialize | DpDREParam::Cmd::Default;
            dre.userId = (((unsigned long long)MEDIA_ISP_PREVIEW)<<32) + (unsigned long long)(sensorID << 8);
            dre.buffer = nullptr;
            P2PlatInfo::NVRamData nvramData = plat->queryNVRamData(P2PlatInfo::FID_DRE, sensorData.mMagic3A, sensorData.mIspProfile);
            dre.p_customSetting = nvramData.mLowIsoData;
            dre.customIndex = nvramData.mLowIsoIndex;
            MSize activeRect = plat->getActiveArrayRect().s;
            dre.activeWidth  = activeRect.w;
            dre.activeHeight = activeRect.h;
        }

        isp.grallocExtraHandle = nullptr;
        if( configInfo.mUsageHint.mIsHdr10 )
        {
            isp.recordType = transHdr10SpecToScenario(configInfo.mUsageHint.mHdr10Spec);
        }
    }

    TRACE_FUNC_EXIT();
    return pq;
}

MVOID setPQDebugName(char *str, size_t size, const char* name)
{
    if( str && size )
    {
        const size_t len = std::min<size_t>(strlen(name)+1, size);
        memcpy(str, name, len-1);
        str[len] = '\0';
    }
}

buffer_handle_t* getBufferHandlePtr(IImageBuffer *buffer)
{
    if( buffer == NULL )
    {
        return NULL;
    }
    IImageBufferHeap *heap = buffer->getImageBufferHeap();
    if( heap == NULL )
    {
        return NULL;
    }
    IGraphicImageBufferHeap *graphicHeap = IGraphicImageBufferHeap::castFrom(heap);
    if( graphicHeap == NULL )
    {
        return NULL;
    }
    return graphicHeap->getBufferHandlePtr();
};

DpPqParam makeDpPqParam(const PQCtrl *pqCtrl, P2IO::TYPE type, IImageBuffer *buffer, const char* caller)
{
    eP2_PQ_PATH pqPath = toP2PQPath(type);

    DpPqParam param;
    param.enable = 0;
    if( pqPath == P2_PQ_PATH_RECORD )
    {
        param.scenario = MEDIA_ISP_RECORD;
    }
    else
    {
        param.scenario = MEDIA_ISP_PREVIEW;
    }
    param.u.isp.userString[0] = '\0';

    if( pqCtrl )
    {
        if( pqCtrl->getLogLevel() >= 1 )
        {
            printPQCtrl("makeDpPq", pqCtrl, caller);
        }

        pqCtrl->fillPQInfo(&param);
        param.u.isp.enableDump = pqCtrl->isDump();
        const char *dbgName = pqCtrl->getDebugName();
        setPQDebugName(param.u.isp.userString, sizeof(param.u.isp.userString),
                       dbgName ? dbgName : "");

        if( pqPath == P2_PQ_PATH_RECORD )
        {
            param.scenario = MEDIA_ISP_RECORD; // was overwritten by fillPQInfo()
        }

        if( !pqCtrl->isDummy() && pqCtrl->supportPQ() )
        {
            if( pqCtrl->needDefaultPQ(pqPath) )
            {
                param.enable = true;
            }
            if( pqCtrl->needCZ(pqPath) )
            {
                param.enable |= (PQ_COLOR_EN|PQ_ULTRARES_EN);
            }
            if( pqCtrl->needHFG(pqPath) )
            {
                param.enable |= (PQ_HFG_EN);
                param.u.isp.dpHFGParam.p_slkParam = pqCtrl->getIspTuning();
            }
            if( pqCtrl->needDRE(pqPath) )
            {
                param.enable |= (PQ_DRE_EN);
                param.u.isp.dpDREParam.userId = (((unsigned long long)MEDIA_ISP_PREVIEW)<<32) + (unsigned long long) (pqCtrl->getSensorID() << 8);
                if( pqCtrl->is2ndDRE() )
                {
                    param.u.isp.dpDREParam.userId += 1;
                }
            }
            param.u.isp.p_faceInfor = pqCtrl->getFDdata();

            if( pqCtrl->needHDR10(pqPath) )
            {
                param.enable |= (PQ_ISP_HDR_EN);
                buffer_handle_t* bufferHandlePtr = getBufferHandlePtr(buffer);
                if( bufferHandlePtr != NULL )
                {
                    param.u.isp.grallocExtraHandle = *bufferHandlePtr;
                }
                else
                {
                    param.u.isp.recordType = RECORD_NONLINEAR;
                    param.u.isp.grallocExtraHandle = nullptr;
                }
            }
            param.u.isp.LCE = pqCtrl->getDreTuning();
            param.u.isp.LCE_Size = (int)pqCtrl->getDreTuningSize();
        }

        if( pqCtrl->getLogLevel() >= 2 )
        {
            printDpPqParam(ILog(), 0, &param, caller ? caller : pqCtrl->getDebugName());
        }
    }
    return param;
}

MVOID updateExpectEndTime(DIPParams &dipParams, MUINT32 ms, MUINT32 expectFps)
{
    if(expectFps == 0)
    {
        struct timeval diff, curr, next;
        diff.tv_sec = 0;
        diff.tv_usec = ms * 1000;
        gettimeofday(&curr, NULL);
        timeradd(&curr, &diff, &next);
        for( unsigned i = 0, n = dipParams.mvDIPFrameParams.size(); i < n; ++i )
        {
            dipParams.mvDIPFrameParams.at(i).ExpectedEndTime = next;
        }
    }
    else
    {
        struct timeval next;
        next.tv_sec = 0;
        next.tv_usec = 0;
        for( unsigned i = 0, n = dipParams.mvDIPFrameParams.size(); i < n; ++i )
        {
            dipParams.mvDIPFrameParams.at(i).ExpectedEndTime = next;
        }
    }
}

MVOID updateScenario(DIPParams &dipParams, MUINT32 fps, MUINT32 resolutionWidth)
{
    for( unsigned i = 0, n = dipParams.mvDIPFrameParams.size(); i < n; ++i )
    {
        DIPFrameParams &param = dipParams.mvDIPFrameParams.at(i);
        param.Fps = fps;
        param.Resolution_Width = resolutionWidth;
    }
}

MVOID push_PQParam(DIPFrameParams &frame, const P2ObjPtr &obj)
{
    TRACE_FUNC_ENTER();
    push_PQParam(frame, (void*)obj.pqParam);
    TRACE_FUNC_EXIT();
}

MVOID push_PQParam(DIPFrameParams &frame, void* pqParam)
{
    TRACE_FUNC_ENTER();
    ExtraParam extra;
    extra.CmdIdx = NSIoPipe::EPIPE_MDP_PQPARAM_CMD;
    extra.moduleStruct = pqParam;
    frame.mvExtraParam.push_back(extra);
    TRACE_FUNC_EXIT();
}

MVOID push_TimgoParam(DIPFrameParams &frame, const P2ObjPtr &obj)
{
    ExtraParam extra;
    extra.CmdIdx = NSIoPipe::EPIPE_TIMGO_DUMP_SEL_CMD;
    extra.moduleStruct = (void*)obj.timgoParam;
    frame.mvExtraParam.push_back(extra);
}

MVOID updateDIPParams(DIPParams &dipParams, const P2Pack &p2Pack, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning)
{
    updateDIPFrameParams(dipParams.mvDIPFrameParams.at(0), p2Pack, io, obj, tuning);
}

DIPParams makeDIPParams(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning)
{
    DIPParams dipParams;
    dipParams.mvDIPFrameParams.push_back(makeDIPFrameParams(p2Pack, tag, io, obj, tuning));
    return dipParams;
}

MVOID updateDIPFrameParams(DIPFrameParams &frame, const P2Pack &p2Pack, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning)
{
    TRACE_S_FUNC_ENTER(p2Pack.mLog);

    const P2PlatInfo* platInfo = p2Pack.getPlatInfo();

    MSize inSize = io.mIMGI.getImgSize();
    for( auto &in : frame.mvIn )
    {
        if( is(in, PORT_IMGI) && in.mBuffer )
        {
            inSize = in.mBuffer->getImgSize();
            break;
        }
    }

    if( tuning.pRegBuf )
    {
        frame.mTuningData = tuning.pRegBuf;
    }
    if( tuning.pLsc2Buf )
    {
        push_in(frame, platInfo->getLsc2Port(), (IImageBuffer*)tuning.pLsc2Buf);
    }
    if( tuning.pBpc2Buf )
    {
        push_in(frame, platInfo->getBpc2Port(), (IImageBuffer*)tuning.pBpc2Buf);
    }

    if( tuning.pLcsBuf )
    {
        IImageBuffer *lcso = (IImageBuffer*)tuning.pLcsBuf;
        push_in(frame, PORT_LCEI, lcso);
        if( platInfo->hasYnrLceiPort() )
        {
            push_in(frame, platInfo->getYnrLceiPort(), lcso);
            push_srz4_LCENR(frame, obj.srz4, p2Pack, io.isResized(), inSize, lcso->getImgSize());
        }
    }
    else if( io.mMSFDSWI && platInfo->hasYnrLceiPort() )
    {
        MSize limit = platInfo->getYnrLceiSizeLimit();
        IImageBuffer *lceiIn = IS_LE(io.mMSFDSWI->getImgSize() , limit) ? io.mMSFDSWI : io.mMSF_n_1_DSWI;
        if( !lceiIn )
        {
            MSize n = io.mMSFDSWI->getImgSize();
            MSize n_1 = io.mMSF_n_1_DSWI ? io.mMSF_n_1_DSWI->getImgSize() : MSize();
            MY_LOGW("Can not find YNR LCEI input buffer but DSWI is valid!!, dswi n(%dx%d), n-1(%dx%d)",
                    n.w, n.h, n_1.w, n_1.h);
        }
        else
        {
            push_in(frame, platInfo->getYnrLceiPort(), lceiIn);
            push_srz4_MOTION_NR(frame, obj.srz4, inSize, lceiIn->getImgSize());
        }
    }

    if( tuning.pFaceAlphaBuf && platInfo->hasYnrFacePort())
    {
        IImageBuffer *facei = (IImageBuffer*)tuning.pFaceAlphaBuf;
        push_in(frame, platInfo->getYnrFaceiPort(), (IImageBuffer*)tuning.pFaceAlphaBuf);
        push_srz3_FACENR(frame, obj.srz3, inSize, facei->getImgSize());
    }

    if( tuning.bDCES_Enalbe )
    {
        if(io.mDCESO.mBuffer)
        {
            initBuffer(io.mDCESO.mBuffer);
            io.mDCESO.mBuffer->syncCache(eCACHECTRL_FLUSH);
            push_out(frame, PORT_DCESO, io.mDCESO.mBuffer);
        }
        else
        {
            MY_LOGW("DCES enabled but dceso is NULL !! Need Check!");
        }
    }

    TRACE_S_FUNC_EXIT(p2Pack.mLog);
}

DIPFrameParams makeDIPFrameParams(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning, MBOOL dumpReg)
{
    TRACE_S_FUNC_ENTER(p2Pack.mLog);
    DIPFrameParams fparam;
    if( !p2Pack.isValid() )
    {
        MY_LOGE("Invalid p2pack");
    }
    else
    {
        fparam = makeOneDIPFrameParam(p2Pack, tag, io, obj, tuning, dumpReg);
        updateDIPFrameParams(fparam, p2Pack, io, obj, tuning);
    }
    TRACE_S_FUNC_EXIT(p2Pack.mLog);
    return fparam;
}

DIPFrameParams makeOneDIPFrameParam(const P2Pack &p2Pack, ENormalStreamTag tag, const P2IOPack &io, const P2ObjPtr &obj, const TuningParam &tuning, MBOOL dumpReg)
{
    const ILog &log = p2Pack.mLog;
    TRACE_S_FUNC_ENTER(log);

    DIPFrameParams frame;
    if( !p2Pack.isValid() )
    {
        MY_S_LOGE(log, "Invalid p2pack=%d", p2Pack.isValid());
    }
    else
    {
        frame.FrameNo = p2Pack.getFrameData().mMWFrameNo;
        frame.RequestNo = p2Pack.getFrameData().mMWFrameRequestNo;
        frame.Timestamp = p2Pack.getSensorData().mP1TS;
        frame.UniqueKey = p2Pack.getSensorData().mMWUniqueKey;
        frame.mStreamTag = tag;
        frame.mSensorIdx = p2Pack.getSensorData().mSensorID;
        frame.SensorDev = p2Pack.getSensorData().mNDDHint.SensorDev;
        frame.IspProfile = (obj.profile >= 0) ? obj.profile : (MINT32) p2Pack.getSensorData().mIspProfile;
        frame.mRunIdx = obj.run;
        frame.NeedDump = dumpReg;

        if( io.mMSFDSI && obj.msfConfig &&
            obj.msfConfig->scale_Total > 0 )
        {
            push_msfIn(frame, io, tuning, obj.msfConfig);
        }
        else if( io.mIMGI.isValid() )
        {
            push_in(frame, PORT_IMGI, io.mIMGI);
        }

        if( io.mVIPI.isValid() )
        {
            push_in(frame, PORT_VIPI, io.mVIPI);
        }

        if( io.mIMG2O.isValid() )
        {
            push_out(frame, PORT_IMG2O, io.mIMG2O);
            push_crop(frame, CROP_IMG2O, io.mIMG2O.mCropRect, io.mIMG2O.getImgSize(), io.mIMG2O.mDMAConstrain);
        }
        if( io.mIMG3O.isValid() )
        {
            push_out(frame, PORT_IMG3O, io.mIMG3O);
        }
        if( io.mWDMAO.isValid() )
        {
            push_out(frame, PORT_WDMAO, io.mWDMAO);
            push_crop(frame, CROP_WDMAO, io.mWDMAO.mCropRect, io.mWDMAO.getImgSize(), io.mWDMAO.mDMAConstrain);
        }
        if( io.mWROTO.isValid() )
        {
            push_out(frame, PORT_WROTO, io.mWROTO);
            push_crop(frame, CROP_WROTO, io.mWROTO.mCropRect, io.mWROTO.getImgSize(), io.mWROTO.mDMAConstrain);
        }
        if( io.mTIMGO.isValid() )
        {
            io.mTIMGO.mBuffer->setExtParam(io.mIMGI.getImgSize());
            push_out(frame, PORT_TIMGO, io.mTIMGO);
            push_TimgoParam(frame, obj);
        }
        if( io.mWDMAO.isValid() || io.mWROTO.isValid() )
        {
            push_PQParam(frame, obj);
        }
    }

    TRACE_S_FUNC_EXIT(log);
    return frame;
}

#define P2RB_DUMP_FILE 0
#define CAMERA_DUMP_ROOT_DIR "/data/vendor/camera_dump"
#define P2RB_TEST_FILE_MAX_LEN 512
void debug_p2rb_saveToFile(
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    MBOOL needSeparateFiles, const string &key, IImageBuffer *pImgBuf)
{
    (void)nddHint;
    (void)needSeparateFiles;
    (void)key;
    (void)pImgBuf;
#if P2RB_DUMP_FILE
    if (pImgBuf == nullptr)
    {
        MY_LOGE("p2rb: !!err: image buffer can't be null");
        return;
    }
    char testFileName[P2RB_TEST_FILE_MAX_LEN];
    char *pTmpUniqueKey = nullptr, *pTmpExtFile = nullptr;
    if ( needSeparateFiles )
    {
        pTmpUniqueKey = "111111111";
        pTmpExtFile = "packed_word";
    }
    else
    {
        pTmpUniqueKey = "888888888";
        if ( key.find("vipi") != std::string::npos )
            pTmpExtFile = "yv12";
        else
            pTmpExtFile = "unknown";
    }

    MINT imgFmt = pImgBuf->getImgFormat();
    MINT imgW = pImgBuf->getImgSize().w;
    MINT imgH = pImgBuf->getImgSize().h;
    MINT stride = pImgBuf->getBufStridesInBytes(0);
    snprintf(testFileName, sizeof(testFileName)-1, "%s/%s-%.4d-%.4d-main-out-%s-PW%d-PH%d-BW%d__%dx%d_%d_s0.%s",
        CAMERA_DUMP_ROOT_DIR, pTmpUniqueKey, nddHint.FrameNo, nddHint.RequestNo, key.c_str(),
        imgW, pImgBuf->getBufSizeInBytes(0)/stride, stride,
        imgW, imgH, pImgBuf->getPlaneBitsPerPixel(0),
        pTmpExtFile);
    MY_LOGD("p2rb: read_key(%s), write_test_file('%s')", key.c_str(), testFileName);
    pImgBuf->saveToFile(testFileName);
#endif // P2RB_DUMP_FILE
}

static void p2rb_printImgFmtInfo(const MINT imgFmt)
{
    MY_LOGD("p2rb: imgFmt(0x%08x), "
        "eImgFmt_MTK_YUV_P210(0x%08x), "
        "eImgFmt_MTK_YVU_P210(0x%08x), "
        "eImgFmt_MTK_YUV_P210_3PLANE(0x%08x), "
        "eImgFmt_MTK_YUV_P010(0x%08x), "
        "eImgFmt_MTK_YVU_P010(0x%08x), "
        "eImgFmt_MTK_YUV_P010_3PLANE(0x%08x), "
        "eImgFmt_MTK_YUV_P012(0x%08x), "
        "eImgFmt_MTK_YVU_P012(0x%08x), ",
        imgFmt,
        eImgFmt_MTK_YUV_P210,
        eImgFmt_MTK_YVU_P210,
        eImgFmt_MTK_YUV_P210_3PLANE,
        eImgFmt_MTK_YUV_P010,
        eImgFmt_MTK_YVU_P010,
        eImgFmt_MTK_YUV_P010_3PLANE,
        eImgFmt_MTK_YUV_P012,
        eImgFmt_MTK_YVU_P012);
}

static MBOOL p2rb_getReprocInfo(
    const char *pUserName,
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    const MINT32 &magic3A, const std::string &key,
    TUNING_REPROCESS_CFG_T &outCfgInfo)
{
    FileReadRule frr;
    MINT idx  = frr.getReprocIndex(magic3A);
    bool infoRet = (idx >= 0) ? frr.getReprocInfo(idx, outCfgInfo) : false;
    MY_LOGD("p2rb: (u%d,f%d,r%d,mn%d): user(%s), key(%s): query Info at idx(%d): %s",
        nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A, pUserName, key.c_str(),
        idx, (infoRet ? "OK" : "NG") );
    return infoRet;
}

static MBOOL p2rb_needSeparateFiles(const MINT imgFmt)
{
    return imgFmt == eImgFmt_MTK_YUV_P210
           || imgFmt == eImgFmt_MTK_YVU_P210
           || imgFmt == eImgFmt_MTK_YUV_P210_3PLANE
           || imgFmt == eImgFmt_MTK_YUV_P010
           || imgFmt == eImgFmt_MTK_YVU_P010
           || imgFmt == eImgFmt_MTK_YUV_P010_3PLANE
           || imgFmt == eImgFmt_MTK_YUV_P012
           || imgFmt == eImgFmt_MTK_YVU_P012;
}

static MBOOL p2rb_prepareFileNamesToLoad(
    const char *pUserName,
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    const MINT32 &magic3A, const std::string &key,
    const MINT imgFmt, const MBOOL needSeparateFiles,
    TUNING_REPROCESS_CFG_T &cfgInfo,
    std::vector<std::string> &outFiles)
{
    // prepare tags for querying files
    MBOOL ret = MTRUE;
    std::vector<std::string> tags;
    if ( needSeparateFiles )
    {
        if ( key.find("msf_") != std::string::npos || key.find("raw") != std::string::npos
             || key.find("omcframe") != std::string::npos || key.find("yuvo") != std::string::npos )
        {
            tags.push_back(key+"_y");
            tags.push_back(key+"_c");
        }
        else
        {
            MY_LOGW("p2rb: (u%d,f%d,r%d,mn%d): user(%s), key(%s): unknown fmt(0x%x)",
                nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A, pUserName, key.c_str(), imgFmt);
            p2rb_printImgFmtInfo(imgFmt);
            ret = MFALSE;
        }
    }
    else
    {
        tags.push_back(key);
    }
    // prepare files to load
    vector<std::string>::iterator itTag;
    for ( itTag = tags.begin(); itTag != tags.end(); itTag++ )
    {
        std::map<std::string, std::string>::iterator itInfo;
        itInfo = cfgInfo.mapFiles.find(*itTag);
        if ( itInfo != cfgInfo.mapFiles.end() )
        {
            outFiles.push_back(itInfo->second);
        }
        else
        {
            outFiles.push_back("unknown");
            ret = MFALSE;
        }
        MY_LOGD("p2rb: (u%d,f%d,r%d,mn%d): user(%s): key(%s): file('%s')",
            nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A, pUserName,
            key.c_str(), (itInfo != cfgInfo.mapFiles.end()) ? itInfo->second.c_str() : "unknown");
    }
    return ret;
}

static MBOOL p2rb_loadFilesToBuf(
    const char *pUserName,
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    const MBOOL bReadbackBuf, const MINT32 &magic3A,
    const std::string &key, const MBOOL needSeparateFiles,
    const std::vector<std::string> &files,
    IImageBuffer *out_pImgBuf)
{
    MBOOL ret = MTRUE;
    if ( bReadbackBuf && files.size() )
    {
        out_pImgBuf->unlockBuf(pUserName);
        if ( needSeparateFiles )
            ret = out_pImgBuf->loadFromFiles(files);
        else
            ret = out_pImgBuf->loadFromFile( files[0].c_str() );

        if ( !ret )
        {
            MY_LOGW("p2rb: !!warn: (u%d,f%d,r%d,mn%d): user(%s): key(%s): readback buf fail: %s",
                nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A, pUserName,
                key.c_str(), (*files.begin()).c_str() );
        }
        const auto __usage = (eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_HW_CAMERA_READWRITE);
        out_pImgBuf->lockBuf(pUserName, __usage);
        out_pImgBuf->syncCache(eCACHECTRL_FLUSH);

        if ( ret )
            debug_p2rb_saveToFile(nddHint, needSeparateFiles, key, out_pImgBuf);
    }

    return ret;
}


MVOID readbackBuf(
    const char *pUserName,
    const TuningUtils::FILE_DUMP_NAMING_HINT &nddHint,
    const MBOOL bReadbackBuf, const MINT32 &magic3A,
    const std::string &key, IImageBuffer *out_pImgBuf)
{
    // error pre-check
    if (pUserName == NULL || out_pImgBuf == NULL)
    {
        MY_LOGE("p2rb: (u%d,f%d,r%d,mn%d): invalid user(%p) or imgBuf(%p)",
            nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A,
            pUserName, out_pImgBuf);
        return;
    }

    TUNING_REPROCESS_CFG_T cfgInfo;
    if (p2rb_getReprocInfo(pUserName, nddHint, magic3A, key, cfgInfo) != MTRUE)
        return;

    MBOOL ret = MTRUE;
    MINT imgFmt = out_pImgBuf->getImgFormat();
    MBOOL needSeparateFiles = p2rb_needSeparateFiles(imgFmt);

    std::vector<std::string> files;
    ret = p2rb_prepareFileNamesToLoad(pUserName, nddHint, magic3A, key,
         imgFmt, needSeparateFiles, cfgInfo, files);

    if (ret)
    {
        ret = p2rb_loadFilesToBuf(pUserName, nddHint, bReadbackBuf, magic3A,
            key, needSeparateFiles, files, out_pImgBuf);
    }
    // debug log
    MY_LOGD("p2rb: (u%d,f%d,r%d,mn%d): user(%s): key(%s): final_result: %s",
        nddHint.UniqueKey, nddHint.FrameNo, nddHint.RequestNo, magic3A, pUserName,
        key.c_str(), (ret ? "OK" : "NG") );
}

std::string p2rb_findRawKey(const MINT32 magic3A)
{
    TUNING_REPROCESS_CFG_T cfgInfo;
    FileReadRule frr;
    MINT idx  = frr.getReprocIndex(magic3A);
    bool infoRet = (idx >= 0) ? frr.getReprocInfo(idx, cfgInfo) : false;
    MY_LOGD("query Info at idx(%d): %s", idx, (infoRet ? "OK" : "NG"));
    if (!infoRet)
        return std::string("unknown");

    if ( cfgInfo.mapFiles.find(std::string("yuvo_y")) != cfgInfo.mapFiles.end() )
        return std::string("yuvo");
    else if ( cfgInfo.mapFiles.find(std::string("raw")) != cfgInfo.mapFiles.end() )
        return std::string("raw");
    else
        return std::string("unknown");
}

} // namespace P2Util
} // namespace Feature
} // namespace NSCam
