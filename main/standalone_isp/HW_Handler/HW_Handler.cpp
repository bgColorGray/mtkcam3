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
#include "HW_Handler_WPE.h"
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
RequestData::
RequestData(std::string Name, struct RequestParams &param)
    : mRequestNo(param.requestNo)
    , mpAppCallback(param.pAppCallback)
    , mpDataCallback(param.pDataCallback)
    , mUserName(Name)
    , mDataLock()
{
    MY_LOGD("new a base request data");
    if (param.pAppControl != nullptr)
    {
        mAppControl = *(param.pAppControl);
    }
    mvInputBuffer = param.inBufs;
    mvOutputBuffer = param.outBufs;
    mvLockedBuffer.clear();
}

RequestData::
~RequestData()
{
    if (mvLockedBuffer.size() != 0)
    {
        MY_LOGE("some buffer is unlocked, force unlock them");
        unlockAllBuffer();
    }
    mvInputBuffer.clear();
    mvOutputBuffer.clear();
    MY_LOGD("delete a base request data");
}

auto
RequestData::
processCallback(
    IMetadata *pMeta,
    int32_t ret
) -> int32_t
{
    sp<IAppCallback> pAppCallback = mpAppCallback.promote();
    sp<IDataCallback> pDataCallback = mpDataCallback.promote();
    if (pDataCallback != nullptr && pMeta != nullptr)
    {
        pDataCallback->onMetaCallback(mRequestNo, 0, 0, *pMeta, ret == 0 ? false : true);
    }
    if (pAppCallback != nullptr)
    {
        IAppCallback::Result result;
        result.frameNo       = mRequestNo;
        result.bFrameEnd     = true;
        result.bResultError  = ret == 0 ? false : true;
        pAppCallback->updateFrame(mRequestNo, 0, result);
    }
    return 0;
}

auto
RequestData::
lockBuffer(
    android::sp<IImageStreamBuffer> streamBuf,
    MBOOL isRead
) -> sp<IImageBuffer>
{
    Mutex::Autolock _l(mDataLock);
    if (streamBuf != nullptr)
    {
        //
        BufferData data;
        if ( mvLockedBuffer.find(streamBuf->getStreamInfo()->getStreamId()) != mvLockedBuffer.end() )
        {
            MY_LOGD("lock buffer again : 0x%llX", streamBuf->getStreamInfo()->getStreamId());
            return mvLockedBuffer[streamBuf->getStreamInfo()->getStreamId()].pImageBuffer;
        }
        data.pStreamBuffer = streamBuf;
        if (isRead)
        {
            data.pBufferHeap = streamBuf->tryReadLock(mUserName.c_str());
        }
        else
        {
            data.pBufferHeap = streamBuf->tryWriteLock(mUserName.c_str());
        }
        MY_LOGI("lock heap : %p", data.pBufferHeap.get());
        if (data.pBufferHeap == nullptr)
        {
            MY_LOGW("pBufferHeap == nullptr");
            goto lbExit;
        }
        if ( data.pBufferHeap->getImgFormat() == eImgFmt_BLOB )
        {
            size_t bufStride[3];
            bufStride[0] = streamBuf->getStreamInfo()->getBufPlanes().planes[0].rowStrideInBytes;
            bufStride[1] = streamBuf->getStreamInfo()->getBufPlanes().planes[1].rowStrideInBytes;
            bufStride[2] = streamBuf->getStreamInfo()->getBufPlanes().planes[2].rowStrideInBytes;
            data.pImageBuffer = data.pBufferHeap->createImageBuffer_FromBlobHeap(
                                                    0,
                                                    streamBuf->getStreamInfo()->getImgFormat(),
                                                    streamBuf->getStreamInfo()->getImgSize(),
                                                    bufStride
                                                    );
        }
        else
        {
            data.pImageBuffer = data.pBufferHeap->createImageBuffer();
        }
        if (data.pImageBuffer == nullptr)
        {
            MY_LOGW("pImageBuffer == nullptr");
            goto lbExit;
        }
        MY_LOGI("create buffer : %p", data.pImageBuffer.get());

        MUINT const usage = eBUFFER_USAGE_SW_READ_OFTEN |
            eBUFFER_USAGE_HW_CAMERA_READWRITE
            ;
        data.pImageBuffer->lockBuf(mUserName.c_str(), usage);

        mvLockedBuffer.emplace(streamBuf->getStreamInfo()->getStreamId(), data);

        return data.pImageBuffer;

    }
lbExit:
    return nullptr;
}

auto
RequestData::
unlockBuffer(
    android::sp<IImageStreamBuffer> streamBuf
) -> int32_t
{
    Mutex::Autolock _l(mDataLock);
    if (streamBuf != nullptr)
    {
        auto iter = mvLockedBuffer.find(streamBuf->getStreamInfo()->getStreamId());
        if (iter != mvLockedBuffer.end())
        {
            auto& data = iter->second;
            if  ( data.pStreamBuffer != nullptr && data.pBufferHeap != nullptr && data.pImageBuffer != nullptr )
            {
                MY_LOGI("unlock buffer : %p, heap : %p ", data.pImageBuffer.get(), data.pBufferHeap.get());
                data.pImageBuffer->unlockBuf(mUserName.c_str());
                data.pStreamBuffer->unlock(mUserName.c_str(), data.pBufferHeap.get());
            }
            mvLockedBuffer.erase(iter);
        }
    }
    MY_LOGI("unlock buufer : 0x%llX", streamBuf->getStreamInfo()->getStreamId());
    return 0;
}

auto
RequestData::
unlockAllBuffer(
) -> int32_t
{
    if (mvLockedBuffer.size() == 0)
    {
        return 0;
    }
    for (auto& b : mvInputBuffer)
    {
        auto& buf = b.second;
        unlockBuffer(buf);
    }
    for (auto& b : mvOutputBuffer)
    {
        auto& buf = b.second;
        unlockBuffer(buf);
    }
    return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
HW_Handler::
HW_Handler(const char* Name)
{
    MY_LOGD("new a base hw handler");
    if (Name != nullptr)
    {
        mHandlerName = Name;
    }
    else
    {
        mHandlerName = "Default_HWHandler";
    }
}

HW_Handler::
~HW_Handler()
{
    MY_LOGD("delete a base hw handler");
    mvInputInfo.clear();
    mvOutputInfo.clear();
}

auto
HW_Handler::
init(
    const IMetadata& param,
    const std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>& vImageInfoMap
) -> int32_t
{
    for (auto const& s : vImageInfoMap)
    {
        auto const& pStreamInfo = s.second;
        if  ( CC_LIKELY(pStreamInfo != nullptr) )
        {
            if  (pStreamInfo->getStreamType() == eSTREAMTYPE_IMAGE_IN)
            {
                mvInputInfo.emplace(s.first, pStreamInfo);
            }
            else
            {
                mvOutputInfo.emplace(s.first, pStreamInfo);
            }
        }
    }
    initHandler(param);
    return 0;
}

auto
HW_Handler::
initHandler(
    const IMetadata& param __unused
) -> int32_t
{
    MY_LOGD("do nothing in base initHandler");
    return 0;
}

auto
HW_Handler::
createReqData(
    struct RequestParams &param
) -> shared_ptr<RequestData>
{
    shared_ptr<RequestData> req = std::make_shared<RequestData>(mHandlerName, param);
    return req;
}

auto
HW_Handler::
getCropInfo(
    const IMetadata* pMetadata,
    MRect &rect
) -> MBOOL
{
    MBOOL ret = tryGetMetadata<MRect>(pMetadata, MTK_SINGLEHW_SETTING_SOURCE_CROP, rect);
    return ret;
}

auto
HW_Handler::
getTransformInfo(
    const IMetadata* pMetadata,
    std::vector<UserTransformData> &transformData
) -> MBOOL
{
    IMetadata::IEntry const& entry = pMetadata->entryFor(MTK_SINGLEHW_SETTING_TRANSFORM);
    MUINT count = entry.count();
    UserTransformData data;
    if ( count == 0 || ((count & 0x01) != 0) )
    {
        MY_LOGD("MTK_SINGLEHW_SETTING_TRANSFORM count(%u) is wrong", entry.count());
        return false;
    }
    for (int i = 0; i < count; i += 2)
    {
        data.streamId  = entry.itemAt(i, Type2Type<MINT32>());
        data.trandform = entry.itemAt(i + 1, Type2Type<MINT32>());
        transformData.push_back(data);
    }
    return true;
}

/******************************************************************************
 *
 ******************************************************************************/

auto
HWHandlerFactory::
createHWHandler(
    int32_t module
) -> android::sp<HW_Handler>
{
    android::sp<HW_Handler> handler = nullptr;
    switch( module )
    {
        case MTK_SINGLEHW_SETTING_MODULE_MDP:
        {
            handler = new HW_Handler_MDP("MDP_Handler");
            break;
        }
#ifdef HANDLER_SUPPORT_WPE
        case MTK_SINGLEHW_SETTING_MODULE_WPE:
        {
            handler = new HW_Handler_WPE("WPE_Handler");
            break;
        }
#endif
        default:
            MY_LOGE("fmt(0x%x) not support", module);
            break;
    }
    return handler;
}
