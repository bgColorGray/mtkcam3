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

#include "HW_Handler.h"
#include "HW_Handler_MDP.h"
#include "../MyUtils.h"

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>

/******************************************************************************
 *
 ******************************************************************************/
using namespace android;
using namespace NSCam;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
using namespace NSCam::v3::isppipeline::model;


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
static bool toDpColorFormat(const NSCam::EImageFormat fmt, DpColorFormat &dpFmt)
{
    bool ret = true;
    switch( fmt )
    {
        case eImgFmt_YUY2:    dpFmt = DP_COLOR_YUYV;      break;
        case eImgFmt_NV21:    dpFmt = DP_COLOR_NV21;      break;
        case eImgFmt_NV12:    dpFmt = DP_COLOR_NV12;      break;
        case eImgFmt_YV12:    dpFmt = DP_COLOR_YV12;      break;
        default:
            MY_LOGE("fmt(0x%x) not support in DP", fmt);
            ret = false;
            break;
    }
    return ret;
}

static void toDpTransform(MUINT32 transform, MUINT32 &rotation, MUINT32 &flip)
{
    switch (transform)
    {
#define TransCase(t, r, f)    \
    case (t):                 \
        rotation = (r);       \
        flip = (f);           \
        break;
    TransCase(0                  , 0   , 0)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_FLIP_H  , 0   , 1)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_FLIP_V  , 180 , 1)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90  , 90  , 0)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_180 , 180 , 0)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90_FLIP_H , 270 , 1)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_90_FLIP_V , 90  , 1)
    TransCase(MTK_SINGLEHW_SETTING_TRANSFORM_ROT_270 , 270 , 0)
    default:
        MY_LOGE("not supported transform(0x%x)", transform);
        break;
#undef TransCase
    }
}
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

/******************************************************************************
 *
 ******************************************************************************/
HW_Handler_MDP::
HW_Handler_MDP(const char* Name)
    : HW_Handler(Name)
{
    MY_LOGD("new a MDP hw handler");
}

HW_Handler_MDP::
~HW_Handler_MDP()
{
    if (mInited)
    {
        uninit();
    }
}

auto
HW_Handler_MDP::
initHandler(
    const IMetadata& param
) -> int32_t
{
    int32_t VideoStream = -1;
    int32_t maxOutPortNum = DpIspStream::queryMultiPortSupport(DpIspStream::ISP_ZSD_STREAM);
    if (mvInputInfo.size() != 1)
    {
        MY_LOGE("Only support one input stream");
        return -1;
    }
    if (mvOutputInfo.size() > maxOutPortNum)
    {
        MY_LOGE("too many output streams, max : %d, current : %zu", maxOutPortNum, mvOutputInfo.size());
        return -1;
    }
    if (mpDpIspStream == nullptr)
    {
        mpDpIspStream = new DpIspStream(DpIspStream::ISP_ZSD_STREAM);
        if (mpDpIspStream == nullptr)
        {
            MY_LOGE("new MDP module failed");
            return -1;
        }
    }
    tryGetMetadata<MINT32>(&param, MTK_SINGLEHW_SETTING_VIDEO_STREAM, VideoStream);
    mVideoId = VideoStream;
    for (auto const& s : mvInputInfo)
    {
        auto const& pStreamInfo = s.second;
        if  ( CC_LIKELY(pStreamInfo != nullptr) )
        {
            src_y_pitch = pStreamInfo->getBufPlanes().planes[0].rowStrideInBytes;
            src_uv_pitch = (pStreamInfo->getBufPlanes().count > 1 ? pStreamInfo->getBufPlanes().planes[1].rowStrideInBytes : 0);
        }
    }

    mvReqQueue.clear();
    run("HW_Handler_MDP::initHandler");
    mInited = true;
    return 0;
}

auto
HW_Handler_MDP::
uninit(
) -> int32_t
{
    MY_LOGI("uninit 1");
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
    if (mpDpIspStream)
    {
        delete mpDpIspStream;
        mpDpIspStream = nullptr;
    }
    MY_LOGI("uninit 4");
    return 0;
}

auto
HW_Handler_MDP::
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
HW_Handler_MDP::
requestExit(
) -> void
{
    Mutex::Autolock _l(mReqQueueLock);
    MY_LOGD("requestExit");
    Thread::requestExit();
    mRequestQueueCond.broadcast();

}

auto
HW_Handler_MDP::
threadLoop(
) -> bool
{

    MY_LOGD("ThreadLoop start !!!");
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
            MY_LOGD("request done !!!");
        }
    }
    MY_LOGD("ThreadLoop end !!!");

    return false;
}

auto
HW_Handler_MDP::
processReq(
    shared_ptr<RequestData> req
) -> int32_t
{
    // set source
    int32_t ret = -1;
    IMetadata *pMeta = nullptr;
    for (auto& b : req->mvInputBuffer)
    {
        auto& pstreamBuf = b.second;
        if  ( CC_LIKELY(pstreamBuf != nullptr) )
        {
            sp<IImageBuffer> buffer = req->lockBuffer(pstreamBuf, true);
            MUINT32 size[3] = {0};
            MUINT   planeCount = buffer->getPlaneCount();
            DpColorFormat dpFormat;
            if (!toDpColorFormat((NSCam::EImageFormat)(buffer->getImgFormat()), dpFormat))
            {
                MY_LOGE("unsupport format");
                return -1;
            }
            MY_LOGD("source y pitch : %d, buffer stride[0] : %d", src_y_pitch, buffer->getBufStridesInBytes(0));
            if (mpDpIspStream->setSrcConfig(
                buffer->getImgSize().w,
                buffer->getImgSize().h,
                src_y_pitch != 0 ? src_y_pitch : buffer->getBufStridesInBytes(0),
                src_uv_pitch != 0 ? src_uv_pitch : (buffer->getPlaneCount() > 1 ? buffer->getBufStridesInBytes(1) : 0),
                dpFormat,
                DP_PROFILE_FULL_BT601,
                eInterlace_None,
                NULL,
                false) < 0)
            {
                MY_LOGE("mpDpIspStream->setSrcConfig failed");
                return -1;
            }

#if defined(USE_ION_FD)
            MINT32  fd[3] = {0};
            MUINT32 offset[3] = {0};
            for( MUINT i = 0; i < planeCount; ++i )
            {
                fd[i]       = buffer->getPlaneFD(i);
                offset[i]   = buffer->getPlaneOffsetInBytes(i);
            }
            if (mpDpIspStream->queueSrcBuffer(fd, offset, size, planeCount) < 0)
#else
            void* va[3] = {0};
            MUINT32 mva[3] = {0};
            for( uint32_t i = 0; i < planeCount; ++i )
            {
                va[i]   = (void*)buffer->getBufVA(i);
                mva[i]  = buffer->getBufPA(i);
                size[i] = buffer->getBufSizeInBytes(i);
            }
            if (mpDpIspStream->queueSrcBuffer(va, mva, size, planeCount) < 0)
#endif
            {
                MY_LOGE("DpIspStream->queueSrcBuffer failed");
                req->unlockAllBuffer();
                req->processCallback(pMeta, ret);
                return -1;
            }

        }
    }
    // get transform
    MUINT32 rotate = 0, flip = 0;
    std::vector<UserTransformData> transData;
    if ( getTransformInfo(&(req->mAppControl), transData) == false )
    {
        transData.clear();
    }
    // get crop
    MRect crop;
    getCropInfo(&(req->mAppControl), crop);
    // set destination
    vector<int32_t> portTable;
    //
    struct timeval diff, curr, target;
    timeval *targetPtr = NULL;
    int port = 0;
    sp<IImageBuffer> dumpbuffer = nullptr;
    std::vector<int32_t> out_stride;
    for (auto& b : req->mvOutputBuffer)
    {
        auto& pstreamBuf = b.second;
        auto& streamId = b.first;
        if  ( CC_LIKELY(pstreamBuf != nullptr) )
        {
            sp<IImageBuffer> buffer = req->lockBuffer(pstreamBuf, false);
            dumpbuffer = buffer;
            void* va[3] = {0};
            MUINT32 mva[3] = {0};
            MUINT32 size[3] = {0};
            MUINT   planeCount = buffer->getPlaneCount();
            DpColorFormat dpFormat;
            DP_PROFILE_ENUM profile;
            ImageInfo info;
            MINT32 y_stride = 0, uv_stride = 0;
            if (!toDpColorFormat((NSCam::EImageFormat)(buffer->getImgFormat()), dpFormat))
            {
                MY_LOGE("unsupport format");
                goto lExit;
            }
            profile = (streamId == mVideoId) ? DP_PROFILE_BT601 : DP_PROFILE_FULL_BT601;
            rotate = 0;
            flip = 0;
            for (size_t i = 0; i < transData.size(); i++)
            {
                if (streamId == transData[i].streamId)
                {
                    toDpTransform(transData[i].trandform, rotate, flip);
                    break;
                }
            }
            if( mpDpIspStream->setRotation(port, rotate) )
            {
                MY_LOGE("mpDpIspStream->setRotation failed");
                goto lExit;
            }
            if( mpDpIspStream->setFlipStatus(port, flip) )
            {
                MY_LOGE("mpDpIspStream->setFlipStatus failed");
                goto lExit;
            }
            MY_LOGD("dst y buffer stride[0] : %d, profile : %d", buffer->getBufStridesInBytes(0), profile);
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
            out_stride.push_back((int32_t)streamId);
            out_stride.push_back(y_stride);
            if (mpDpIspStream->setDstConfig(
                port,
                info.width,
                info.height,
                y_stride,
                uv_stride,
                dpFormat,
                profile,
                eInterlace_None,
                NULL,
                false) < 0)
            {
                MY_LOGE("mpDpIspStream->setDstConfig failed");
                goto lExit;
            }
            MY_LOGD("crop : %d, %d, %d, %d", crop.p.x, crop.p.y, crop.s.w, crop.s.h);
            if( mpDpIspStream->setSrcCrop(port, crop.p.x, 0, crop.p.y, 0, crop.s.w, 0, crop.s.h, 0) )
            {
                MY_LOGE("mpDpIspStream->setSrcCrop failed");
                goto lExit;
            }
            for( uint32_t i = 0; i < planeCount; ++i )
            {
                va[i]   = (void*)buffer->getBufVA(i);
                mva[i]  = buffer->getBufPA(i);
                size[i] = buffer->getBufSizeInBytes(i);
            }
            if (mpDpIspStream->queueDstBuffer(port, va, mva, size, planeCount) < 0)
            {
                MY_LOGE("DpIspStream->queueDstBuffer failed");
                goto lExit;
            }
            portTable.push_back(port);
            port++;
        }
    }
    // start MDP
    {
        diff.tv_sec = 0;
        diff.tv_usec = 33 * 1000;
        gettimeofday(&curr, NULL);
        timeradd(&curr, &diff, &target);
        targetPtr = &target;
    }
    if( mpDpIspStream->startStream(targetPtr) < 0 )
    {
        MY_LOGE("DpIspStream->startStream failed");
        goto lExit;
    }
    if( mpDpIspStream->dequeueSrcBuffer() < 0 )
    {
        MY_LOGE("DpIspStream->dequeueSrcBuffer failed");
        goto lExit;
    }
    {
        for ( MUINT32 i = 0; i < portTable.size(); ++i )
        {
            void* va[3] = {0};
            if( mpDpIspStream->dequeueDstBuffer(portTable[i], va) < 0 )
            {
                MY_LOGE("DpIspStream->dequeueDstBuffer[%d] failed", portTable[i]);
                goto lExit;
            }
        }
    }
    if( mpDpIspStream->stopStream() < 0 )
    {
        MY_LOGE("DpIspStream->stopStream failed");
        goto lExit;
    }
    if( mpDpIspStream->dequeueFrameEnd() < 0 )
    {
        MY_LOGE("DpIspStream->dequeueFrameEnd failed");
        goto lExit;
    }
    dumpbuffer = nullptr;
    ret = 0;
    {
        IMetadata::IEntry tag(MTK_SINGLEHW_SETTING_OUT_STREAM_STRIDE);
        pMeta = new IMetadata();
        for (size_t i = 0; i < out_stride.size(); i++)
        {
            tag.push_back(out_stride[i], Type2Type<MINT32>());
        }
        pMeta->update(MTK_SINGLEHW_SETTING_OUT_STREAM_STRIDE, tag);
    }
lExit:
    req->unlockAllBuffer();
    req->processCallback(pMeta, ret);
    if (pMeta != nullptr)
    {
        delete pMeta;
    }
    return 0;
}
