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

#include "../HW_Handler.h"
#include "../HW_Handler_WPE.h"
#include "../../MyUtils.h"

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>

#include <mtkcam/drv/iopipe/PostProc/IWpeStream.h>
#include <mtkcam/drv/iopipe/PostProc/WpeUtility.h>
#include <mtkcam/drv/iopipe/Port.h>

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::isppipeline::model;

using namespace NSCam::NSIoPipe;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::NSIoPipe::NSWpe;

#define WDMAO_CROP_GROUP 2
#define WROTO_CROP_GROUP 3

/******************************************************************************
 *
 ******************************************************************************/

CAM_ULOG_DECLARE_MODULE_ID(MOD_ISP_PIPELINE_MODEL);
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGE("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(...)                MY_ERROR(F, __VA_ARGS__)
//
#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#define MY_LOGD1(...)               MY_LOGD_IF(           (mLogLevel>=1), __VA_ARGS__)
#define MY_LOGD2(...)               MY_LOGD_IF(CC_UNLIKELY(mLogLevel>=2), __VA_ARGS__)
#define MY_LOGD3(...)               MY_LOGD_IF(CC_UNLIKELY(mLogLevel>=3), __VA_ARGS__)

#if 1
#define FUNC_START MY_LOGD("%s +", __FUNCTION__);
#define FUNC_END MY_LOGD("%s -", __FUNCTION__);
#else
#define FUNC_START
#define FUNC_END
#endif

/******************************************************************************
 *
 ******************************************************************************/
struct ImageInfo
{
    MINT32 format;
    MINT32 width;
    MINT32 height;
    MINT32 y_plane_size = 0;
    MINT32 uv_plane_size = 0;
};
static void getImageStride(ImageInfo info, MINT32 &y_stride, MINT32 &uv_stride)
{
    switch (info.format)
    {
        case eImgFmt_YUY2:
        {
            y_stride = info.width*2;
            uv_stride = 0;
            break;
        }
        case eImgFmt_NV21:
        case eImgFmt_NV12:
        {
            y_stride = info.width;
            uv_stride = info.width;
            break;
        }
        case eImgFmt_YV12:
        {
            y_stride = info.width;
            uv_stride = info.width >> 1;
            break;
        }
        default:
        {
            y_stride = 0;
            uv_stride = 0;
            MY_LOGE("fmt(0x%x) not support in DP", info.format);
            break;
        }
    }
    if (y_stride*info.height > info.y_plane_size)
    {
        MY_LOGE("get stride failed, stride : %d, height : %d, y_plane_size : %d", y_stride, info.height, info.y_plane_size);
        y_stride = 0;
        uv_stride = 0;
    }
}

static void toTransform(MUINT32 transform, MUINT32 &rotation, MUINT32 &flip, MUINT32 &mtkTransform)
{
    switch (transform)
    {
#define TransCase(t, r, f, mtkT)    \
    case (t):                 \
        rotation = (r);       \
        flip = (f);           \
        mtkTransform = (mtkT);   \
        break;
    TransCase(0                                      , 0   , 0, 0)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_FLIP_H  , 0   , 1, eTransform_FLIP_H)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_FLIP_V  , 180 , 1, eTransform_FLIP_V)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90  , 90  , 0, eTransform_ROT_180)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_180 , 180 , 0, eTransform_ROT_90)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90_FLIP_H , 270 , 1, eTransform_FLIP_H|eTransform_ROT_90)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90_FLIP_V , 90  , 1, eTransform_FLIP_V|eTransform_ROT_90)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_270 , 270 , 0, eTransform_ROT_270)
    default:
        MY_LOGE("not supported transform(0x%x)", transform);
        break;
#undef TransCase
    }
}

static void getImageSize(NSCam::EImageFormat fmt, MSize Size, MINT32 &imgSz)
{
    switch (fmt)
    {
        case eImgFmt_YUY2:
        {
            imgSz = Size.w*Size.h*2;
            break;
        }
        case eImgFmt_NV21:
        case eImgFmt_NV12:
        {
            imgSz = Size.w*Size.h*3/2;
            break;
        }
        default:
        {
            imgSz = 0;
            MY_LOGE("fmt(0x%x) not support", fmt);
            break;
        }
    }
}


/******************************************************************************
* save the buffer to the file
*******************************************************************************/
static bool
saveBufToFile(char *const fname, char *const buf, MUINT32 const size)
{
    int nw, cnt = 0;
    uint32_t written = 0;

    //LOG_INF("(name, buf, size) = (%s, %x, %d)", fname, buf, size);
    //LOG_INF("opening file [%s]\n", fname);
    int fd = ::open(fname, O_RDWR | O_CREAT, S_IRWXU);
    if (fd < 0) {
        printf(": failed to create file [%s]: %s \n", fname, ::strerror(errno));
        return false;
    }

    //LOG_INF("writing %d bytes to file [%s]\n", size, fname);
    while (written < size) {
        nw = ::write(fd,
                     buf + written,
                     size - written);
        if (nw < 0) {
            printf(": failed to write to file [%s]: %s\n", fname, ::strerror(errno));
            break;
        }
        written += nw;
        cnt++;
    }
    //LOG_INF("done writing %d bytes to file [%s] in %d passes\n", size, fname, cnt);
    ::close(fd);
    return true;
}


/******************************************************************************
 *
 ******************************************************************************/
HW_Handler_WPE::
HW_Handler_WPE(const char* Name)
    : HW_Handler(Name)
{
    MY_LOGD("new a WPE hw handler");
}

HW_Handler_WPE::
~HW_Handler_WPE()
{
    MY_LOGD("delete a WPE hw handler");
    if (mInited)
    {
        uninit();
    }
}

auto
HW_Handler_WPE::
initHandler(
    const IMetadata& param
) -> int32_t
{

#define MAX_INPUT_NUM  4 //WPEI[2], WarpMap_X, WarpMap_Y, WarpMap_Z
#define MAX_OUTPUT_NUM 2 //WDMAO, WROTO

    int32_t VideoStream = -1;

    if (mvInputInfo.size() > MAX_INPUT_NUM)
    {
        MY_LOGE("too many output streams, max : %d, current : %zu", MAX_INPUT_NUM, mvOutputInfo.size());
        return -1;
    }
    if (mvOutputInfo.size() > MAX_OUTPUT_NUM)//DL mode
    {
        MY_LOGE("too many output streams, max : %d, current : %zu", MAX_OUTPUT_NUM, mvOutputInfo.size());
        return -1;
    }
    if (mpStream == nullptr)
    {
        mpStream = NSCam::NSIoPipe::NSPostProc::INormalStream::createInstance(0xFFFF);
        if (mpStream == nullptr)
        {
            MY_LOGE("new WPE module failed");
            return -1;
        }
        mpStream->init("HW_Handler_WPE", NSCam::NSIoPipe::EStreamPipeID_WarpEG);
    }

    for (auto const& s : mvInputInfo)
    {
        auto const& pStreamInfo = s.second;
        if  ( CC_LIKELY(pStreamInfo != nullptr) )
        {
            if ((NSCam::EImageFormat)(pStreamInfo->getImgFormat()) != eImgFmt_BLOB)
            {
                src_y_pitch = pStreamInfo->getBufPlanes().planes[0].rowStrideInBytes;
                src_uv_pitch = (pStreamInfo->getBufPlanes().count > 1 ? pStreamInfo->getBufPlanes().planes[1].rowStrideInBytes : 0);
            }
        }
    }

    mvReqQueue.clear();
    run("HW_Handler_WPE::initHandler");
    mInited = true;
    return 0;
}

auto
HW_Handler_WPE::
uninit(
) -> int32_t
{
    MY_LOGI("uninit+");
    if (mInited)
    {
        requestExit();
        join();
    }
    mInited = false;
    {
        Mutex::Autolock _l(mReqQueueLock);
        if (!mvReqQueue.empty())
        {
            MY_LOGE("request queue is not empty, something wrong!!");
            mvReqQueue.clear();
        }
    }
    if (mpStream != nullptr)
    {
        mpStream->uninit("HW_Handler_WPE", NSCam::NSIoPipe::EStreamPipeID_WarpEG);
        mpStream->destroyInstance();
        mpStream = nullptr;
    }
    MY_LOGI("-");
    return 0;
}

auto
HW_Handler_WPE::
WPECallback_Pass(
    QParams& rParams
) -> void
{
    IMetadata *pMeta = nullptr;
    EnqData *eQdata = (EnqData *)(rParams.mpCookie);

    MY_LOGD("+");

    if(eQdata == nullptr)
    {
        MY_LOGE("error: rParams.mpCookie is null\n");
        return;
    }

    shared_ptr<RequestData> req = eQdata->req;
    if (req == nullptr)
    {
        MY_LOGE("error: rParams.mpCookie.req is null\n");
        return;
    }

    IMetadata::IEntry tag(MTK_SINGLEHW_SETTING_OUT_STREAM_STRIDE);
    pMeta = new IMetadata();
    for (size_t i = 0; i < eQdata->out_stride.size(); i++)
    {
        tag.push_back(eQdata->out_stride[i], Type2Type<MINT32>());
    }
    eQdata->out_stride.clear();
    delete eQdata;

    if (pMeta != nullptr)
    {
        pMeta->update(MTK_SINGLEHW_SETTING_OUT_STREAM_STRIDE, tag);
    } else {
        MY_LOGE("error: Metadata is null\n");
    }

    //dump
    if(rParams.mvFrameParams.size())
    {
        for (uint32_t i = 0; i < rParams.mvFrameParams[0].mvOut.size(); i++)
        {
            char logbuf[128];
            int n = sprintf(logbuf, "sdcard/wpe_dst_%d.yuv", i);
            if (n < 0)
            {
                MY_LOGW("size of wpe_dst_%d is 0", i);
            }
            Output output = rParams.mvFrameParams[0].mvOut[i];
            MY_LOGD("[mvOut:%d] Port(x%x),p#(%d),va/pa(%p/%p)", i, output.mPortID.index,
                output.mBuffer->getPlaneCount(), output.mBuffer->getBufVA(0),output.mBuffer->getBufPA(0));
            output.mBuffer->saveToFile(logbuf);

            #if 0 //separate dump y & uv
            MINT32 imgSz;
            getImageSize((NSCam::EImageFormat)output.mBuffer->getImgFormat(), output.mBuffer->getImgSize(), imgSz);
            saveBufToFile((char *const)"sdcard/wdmao_y.yuv", (char *const)output.mBuffer->getBufVA(0), imgSz);
            if(output.mBuffer->getPlaneCount() > 1)
                saveBufToFile((char *const)"sdcard/wdmao_uv.yuv", (char *const)output.mBuffer->getBufVA(1), imgSz);
            #endif
        }
    } else {
        MY_LOGE("error: mvFrameParams sz=0!");
    }

    //
    req->unlockAllBuffer();
    req->processCallback(pMeta, 0);
    if (pMeta != nullptr)
    {
        delete pMeta;
    }

    return;
}

auto
HW_Handler_WPE::
enqueReq(
    shared_ptr<RequestData> req
) -> int32_t
{
    Mutex::Autolock _l(mReqQueueLock);

    mvReqQueue.push_back(req);
    mRequestQueueCond.broadcast();

    return 0;
}

auto
HW_Handler_WPE::
requestExit(
) -> void
{
    Mutex::Autolock _l(mReqQueueLock);
    MY_LOGD("requestExit");
    Thread::requestExit();
    mRequestQueueCond.broadcast();

}

auto
HW_Handler_WPE::
threadLoop(
) -> bool
{

    MY_LOGD("+");
    while(!exitPending())
    {
        shared_ptr<RequestData> req = nullptr;
        {
            Mutex::Autolock _l(mReqQueueLock);
            if (mvReqQueue.empty())
            {
                mRequestQueueCond.wait(mReqQueueLock);
            }
            if (!mvReqQueue.empty())
            {
                req = mvReqQueue.front();
                mvReqQueue.pop_front();
            }
        }
        if (req != nullptr)
        {
            processReq(req);
            MY_LOGD("request done");
        }
    }

    MY_LOGD("-");

    return false;
}

auto
HW_Handler_WPE::
processReq(
    shared_ptr<RequestData> req
) -> int32_t
{
    QParams enQParam;
    FrameParams frameParam;
    WPEQParams wpeParam;
    bool ret = true;

    MRect rtWarpOut;
    MRect mdpSrcCrop;
    uint32_t setWDMAO = 0, setWROTO = 0;
    struct timeval diff, curr, target;
    std::vector<UserTransformData> transData;
    ExtraParam exParams;

    EnqData *eQdata = new EnqData();
    eQdata->req = req;

    wpeParam.wpe_mode = NSIoPipe::NSWpe::WPE_MODE_MDP;//fixed: DL mode

    //get warpMap stream id
    MINT32 warpmapx = -1, warpmapy = -1, warpmapz = -1;
    tryGetMetadata<MINT32>(&(req->mAppControl), MTK_SINGLEHW_SETTING_WARP_MAP_X, warpmapx);
    tryGetMetadata<MINT32>(&(req->mAppControl), MTK_SINGLEHW_SETTING_WARP_MAP_Y, warpmapy);
    tryGetMetadata<MINT32>(&(req->mAppControl), MTK_SINGLEHW_SETTING_WARP_MAP_Z, warpmapz);
    wpeParam.vgen_hmg_mode = (warpmapz < 0) ? 0 : 1;
    MY_LOGD("[WarpMap] sId x/y/z(%d/%d/%d)", warpmapx, warpmapy, warpmapz);

    MCrpRsInfo vgenCropInfo;//no use, but need to push one into mwVgenCropInfo
    vgenCropInfo.mCropRect.p_integral.x = 0;
    vgenCropInfo.mCropRect.p_integral.y = 0;
    vgenCropInfo.mCropRect.p_fractional.x = 0;
    vgenCropInfo.mCropRect.p_fractional.y = 0;
    wpeParam.mwVgenCropInfo.push_back(vgenCropInfo);

    // set source
    for (auto& b : req->mvInputBuffer)
    {
        auto& streamId = b.first;
        auto& pstreamBuf = b.second;
        if  ( CC_LIKELY(pstreamBuf != nullptr) )
        {
            sp<IImageBuffer> buffer = req->lockBuffer(pstreamBuf, true);
            MUINT   planeCount = buffer->getPlaneCount();
            if(planeCount > 2)
            {
                MY_LOGE("error: too much plane (%d) > 2", planeCount);
                goto lExit;
            }

            //warpMap
            if((NSCam::EImageFormat)(buffer->getImgFormat()) == eImgFmt_BLOB)
            {
                WarpMatrixInfo *WarpMap = nullptr;
                if(streamId == warpmapx){
                    WarpMap = &(wpeParam.warp_veci_info);
                     buffer->saveToFile("sdcard/wpe_WMx.yuv");
                } else if (streamId == warpmapy) {
                    WarpMap = &(wpeParam.warp_vec2i_info);
                    buffer->saveToFile("sdcard/wpe_WMy.yuv");
                } else if(streamId == warpmapz) {
                    WarpMap = &(wpeParam.warp_vec3i_info);
                    buffer->saveToFile("sdcard/wpe_WMz.yuv");
                } else {
                    MY_LOGE("error: [Src:%d] non-WarpMap,unsupported fmt(x%x)", streamId, eImgFmt_BLOB);
                    goto lExit;
                }
                WarpMap->width = buffer->getImgSize().w;
                WarpMap->height = buffer->getImgSize().h;
                WarpMap->stride = buffer->getBufStridesInBytes(0);//WarpMap->width*4;
                WarpMap->virtAddr = buffer->getBufVA(0);
                WarpMap->phyAddr = buffer->getBufPA(0);
                WarpMap->bus_size = NSCam::NSIoPipe::NSWpe::WPE_BUS_SIZE_32_BITS;
                WarpMap->addr_offset = 0;
                WarpMap->veci_v_flip_en = 0;
            //input
            } else {
                NSCam::NSIoPipe::Input input;
                input.mPortID = PORT_WPEI;
                input.mBuffer = static_cast<IImageBuffer *>(buffer.get());
                input.mPortID.group=0;
                frameParam.mvIn.push_back(input);
                buffer->saveToFile("sdcard/wpe_src.yuv");
            }
            MY_LOGD("[Src:%d] fmt/p#(x%x/%d),w/h/stride(%d/%d/%d),va/pa(%p/%p)",
                    streamId, buffer->getImgFormat(), buffer->getPlaneCount(),
                    buffer->getImgSize().w, buffer->getImgSize().h, buffer->getBufStridesInBytes(0),
                    buffer->getBufVA(0), buffer->getBufPA(0));
        }
    }

    //WPE VGEN out size
    rtWarpOut.clear();
    tryGetMetadata<MRect>(&(req->mAppControl), MTK_SINGLEHW_SETTING_WARP_OUTPUT, rtWarpOut);
    wpeParam.wpecropinfo.x_start_point = rtWarpOut.p.x;
    wpeParam.wpecropinfo.y_start_point = rtWarpOut.p.y;
    wpeParam.wpecropinfo.x_end_point = rtWarpOut.s.w - 1;
    wpeParam.wpecropinfo.y_end_point = rtWarpOut.s.h - 1;
    MY_LOGD("[WarpOutput] x/y(%d/%d),w/h(%d/%d)", rtWarpOut.p.x, rtWarpOut.p.y, rtWarpOut.s.w, rtWarpOut.s.h);

    // get mdp src crop
    getCropInfo(&(req->mAppControl), mdpSrcCrop);
    MY_LOGD("[mdpSrcCrop] x/y(%d/%d),w/h(%d/%d)", mdpSrcCrop.p.x, mdpSrcCrop.p.y, mdpSrcCrop.s.w, mdpSrcCrop.s.h);

    // get transform
    if ( getTransformInfo(&(req->mAppControl), transData) == false )
    {
        transData.clear();
    }

    // set destination
    for (auto& b : req->mvOutputBuffer)
    {
        auto& pstreamBuf = b.second;
        auto& streamId = b.first;
        if  ( CC_LIKELY(pstreamBuf != nullptr) )
        {
            sp<IImageBuffer> buffer = req->lockBuffer(pstreamBuf, false);
            MUINT32 transD = 0, transform = 0, rotate = 0, flip = 0;
            for (size_t i = 0; i < transData.size(); i++)
            {
                if (streamId == transData[i].streamId)
                {
                    transD = transData[i].trandform;
                    toTransform(transData[i].trandform, rotate, flip, transform);
                    break;
                }
            }

            // wdmao or wroto
            MCrpRsInfo cropInfo;
            cropInfo.mFrameGroup=0;
            NSCam::NSIoPipe::Output dst;
            if(!rotate && !flip)
            {
                if(!setWDMAO)
                {
                    setWDMAO = 1;
                    dst.mPortID = PORT_WDMAO;
                    cropInfo.mGroupID = WDMAO_CROP_GROUP;
                } else if(!setWROTO) {
                    MY_LOGW("warning: [dst:%d] no transform, 2nd stream set to WROTO", streamId);
                    setWROTO = 1;
                    dst.mPortID = PORT_WROTO;
                    cropInfo.mGroupID = WROTO_CROP_GROUP;
                }
            } else {
                if(!setWROTO)
                {
                    setWROTO = 1;
                    dst.mPortID = PORT_WROTO;
                    cropInfo.mGroupID = WROTO_CROP_GROUP;
                } else {
                    MY_LOGE("error: [dst:%d] more than 1 stream transform", streamId);
                    goto lExit;
                }
            }
            MY_LOGD("[dst:%d] port/fmt/p#(x%x/x%x/%d),w/h/stride(%d/%d/%d),va/pa(%p/%p),T/T'/R/F(x%x/x%x/%d/%d)",
                streamId, dst.mPortID.index, buffer->getImgFormat(),buffer->getPlaneCount(),
                buffer->getImgSize().w, buffer->getImgSize().h, buffer->getBufStridesInBytes(0),
                buffer->getBufVA(0), buffer->getBufPA(0),
                transD, transform, rotate, flip);

            dst.mPortID.group = 0;
            dst.mBuffer = static_cast<IImageBuffer *>(buffer.get());
            dst.mTransform = transform;
            frameParam.mvOut.push_back(dst);

            //mdp src crop
            cropInfo.mCropRect.s.w = mdpSrcCrop.s.w;
            cropInfo.mCropRect.s.h = mdpSrcCrop.s.h;
            cropInfo.mCropRect.p_integral.x=mdpSrcCrop.p.x;
            cropInfo.mCropRect.p_integral.y=mdpSrcCrop.p.y;
            cropInfo.mCropRect.p_fractional.x=0;
            cropInfo.mCropRect.p_fractional.y=0;
            frameParam.mvCropRsInfo.push_back(cropInfo);

#if 1
            MINT32 y_stride = 0;
            y_stride = buffer->getBufStridesInBytes(0);
#else
          /* WPE use ImageBuffer->getImgSize().w & h directly.
             If rotate 90 or 270 degree, w, h, stride will be changed,
             dst.mBuffer w,h,stride must be that after rotated
          */

            ImageInfo info;
            MINT32 y_stride = 0, uv_stride = 0;

            info.format = buffer->getImgFormat();
            info.y_plane_size = buffer->getBufSizeInBytes(0);
            info.uv_plane_size = (buffer->getPlaneCount() > 1 ? buffer->getBufSizeInBytes(1) : 0);

            if (rotate == 90 || rotate == 270)
            {
                info.width = buffer->getImgSize().h;
                info.height = buffer->getImgSize().w;
                getImageStride(info, y_stride, uv_stride);
            }
            else
            {
                info.width = buffer->getImgSize().w;
                info.height = buffer->getImgSize().h;
                y_stride = buffer->getBufStridesInBytes(0);
                uv_stride = (buffer->getPlaneCount() > 1 ? buffer->getBufStridesInBytes(1) : 0);
            }
#endif
            eQdata->out_stride.push_back((int32_t)streamId);
            eQdata->out_stride.push_back(y_stride);
        }
    }

    diff.tv_sec = 0;
    diff.tv_usec = 33 * 1000;
    gettimeofday(&curr, NULL);
    timeradd(&curr, &diff, &target);
    frameParam.ExpectedEndTime.tv_sec = target.tv_sec;
    frameParam.ExpectedEndTime.tv_usec = target.tv_usec;

    // insert WPE QParam
    exParams.CmdIdx = EPIPE_WPE_INFO_CMD;
    exParams.moduleStruct = (void*) &wpeParam;
    frameParam.mvExtraParam.push_back(exParams);

    enQParam.mpfnCallback = WPECallback_Pass;
    enQParam.mpCookie = (void*)eQdata;
    enQParam.mvFrameParams.push_back(frameParam);

    //enque wpe
    MY_LOGD("wpe->enque+");
    ret = mpStream->enque(enQParam);
    MY_LOGD("wpe->enque-");

    return 0;

lExit:
    eQdata->out_stride.clear();
    delete eQdata;

    req->unlockAllBuffer();
    req->processCallback(nullptr, -1);
    return -1;
}

