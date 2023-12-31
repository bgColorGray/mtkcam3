/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "MtkCam/IBUS"

#include <mtkcam3/feature/utils/ImageBufferUtils.h>
#include <utils/String8.h>

#include <utils/StrongPointer.h>

#include <mtkcam/utils/std/Log.h>
#include <mtkcam/def/UITypes.h>
#include <mtkcam/def/ImageFormat.h>

#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/imgbuf/ImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>

#define BUFFER_USAGE_SW (eBUFFER_USAGE_SW_READ_OFTEN | \
                         eBUFFER_USAGE_SW_WRITE_OFTEN | \
                         eBUFFER_USAGE_HW_CAMERA_READWRITE)

#define ALIGN_FLOOR(x,a)    ((x) & ~((a) - 1L))
#define ALIGN_CEIL(x,a)     (((x) + (a) - 1L) & ~((a) - 1L))

using namespace NSCam;
using namespace NSCam::Utils::Format;
using namespace android;

// ---------------------------------------------------------------------------

ANDROID_SINGLETON_STATIC_INSTANCE(ImageBufferUtils);

MBOOL ImageBufferUtils::allocBuffer(
        sp<IImageBuffer>& imageBuf,
        MUINT32 w, MUINT32 h, MUINT32 format, MBOOL isContinuous, MUINT32 aligned, MBOOL isPixelAligned)
{
    MBOOL ret = MTRUE;

    sp<IImageBuffer> pBuf;

    size_t planeCount = queryPlaneCount(format);

    MINT32 bufBoundaryInBytes[3] = {0};
    MUINT32 strideInBytes[3] = {0};
    for (size_t i = 0; i < planeCount; i++)
    {
        if (isPixelAligned) {
            MUINT32 pixelAlignedWidth = ALIGN_CEIL(w, aligned);
            strideInBytes[i] =
                (queryPlaneWidthInPixels(format, i, pixelAlignedWidth) *
                 queryPlaneBitsPerPixel(format, i) + 7) / 8;
        } else {
            // Bytes aligend
            strideInBytes[i] =
                (queryPlaneWidthInPixels(format, i, w) *
                 queryPlaneBitsPerPixel(format, i) + 7) / 8;
            strideInBytes[i] = ALIGN_CEIL(strideInBytes[i], aligned);
        }
        CAM_LOGD("allocBuffer strideInBytes[%zu] strideInBytes(%d)",
                i, strideInBytes[i]);
    }

    if (isContinuous)
    {
        // to avoid non-continuous multi-plane memory,
        // allocate memory in blob format and map it to ImageBuffer
        MUINT32 allPlaneSize = 0;
        for (size_t i = 0; i < planeCount; i++)
        {
            MUINT32 planeWidthInBytes = 0;
            if (isPixelAligned) {
                MUINT32 pixelAlignedWidth = ALIGN_CEIL(w, aligned);
                planeWidthInBytes = (queryPlaneWidthInPixels(format, i, pixelAlignedWidth) *
                        queryPlaneBitsPerPixel(format, i) + 7) / 8;
            } else {
                // Bytes aligend
                planeWidthInBytes = (queryPlaneWidthInPixels(format, i, w) *
                        queryPlaneBitsPerPixel(format, i) + 7) / 8;
                planeWidthInBytes = ALIGN_CEIL(planeWidthInBytes, aligned);
            }

            allPlaneSize += planeWidthInBytes * queryPlaneHeightInPixels(format, i, h);
        }
        CAM_LOGV("allocBuffer all plane size(%d)", allPlaneSize);

        // allocate blob buffer
        IImageBufferAllocator::ImgParam blobParam(allPlaneSize, bufBoundaryInBytes[0]);

        IImageBufferAllocator *allocator = IImageBufferAllocator::getInstance();
        sp<IImageBuffer> tmpImageBuffer = allocator->alloc(LOG_TAG, blobParam);
        if (tmpImageBuffer == nullptr)
        {
            CAM_LOGF("tmpImageBuffer is NULL");
            return MFALSE;
        }

        // NOTE: after sp holds the allocated buffer, free can be called anywhere
        allocator->free(tmpImageBuffer.get());

        if (!tmpImageBuffer->lockBuf(LOG_TAG, BUFFER_USAGE_SW))
        {
            CAM_LOGF("lock Buffer failed");
            return MFALSE;
        }

        // encapsulate tmpImageBuffer into external ImageBuffer
        IImageBufferAllocator::ImgParam extParam(
                format, MSize(w,h), strideInBytes, bufBoundaryInBytes, planeCount);
        PortBufInfo_v1 portBufInfo =
            PortBufInfo_v1(
                    tmpImageBuffer->getFD(),
                    tmpImageBuffer->getBufVA(0),
                    0, 0, 0);
        portBufInfo.offset[0] = 0;

        sp<ImageBufferHeap> heap =
            ImageBufferHeap::create(LOG_TAG, extParam, portBufInfo);
        if (heap == nullptr)
        {
            CAM_LOGF("pHeap is NULL");
            return MFALSE;
        }

        pBuf = heap->createImageBuffer();

        // add into list for management
        {
            Mutex::Autolock _l(mInternalBufferListLock);
            mInternalBufferList.add(
                    reinterpret_cast<MINTPTR>(pBuf.get()), tmpImageBuffer);
        }
    }
    else
    {
        IIonImageBufferHeap::AllocImgParam_t imgParam(
                format, MSize(w,h), strideInBytes, bufBoundaryInBytes, planeCount);
        sp<IIonImageBufferHeap> heap =
            IIonImageBufferHeap::create(LOG_TAG,
                    imgParam, IIonImageBufferHeap::AllocExtraParam(), MFALSE);
        if (heap == nullptr)
        {
            CAM_LOGF("heap is NULL");
            return MFALSE;
        }

        pBuf = heap->createImageBuffer();
    }

    if (pBuf == nullptr)
    {
        CAM_LOGE("pBuf is NULL");
        return MFALSE;
    }

    if (!pBuf->lockBuf(LOG_TAG, BUFFER_USAGE_SW))
    {
        CAM_LOGE("lock Buffer failed");
        ret = MFALSE;
    }
    else
    {
        // flush
        //pBuf->syncCache(eCACHECTRL_INVALID);  //hw->cpu
        CAM_LOGD("allocBuffer addr(%p) size(%dx%d) format(0x%x)",
                pBuf.get(), w, h, format);

        {
            String8 msg("allocBuffer");
            for (size_t i = 0; i < planeCount; i++)
            {
                msg.appendFormat(
                        " plane:va(%zu:%#" PRIxPTR ")", i, pBuf->getBufVA(i));
            }
            CAM_LOGD("%s", msg.string());
        }
        imageBuf = pBuf;
    }

    return ret;
}

MBOOL ImageBufferUtils::allocBuffer(
            const BufferInfo& info,
            sp<IImageBuffer>& imageBuf,
            std::shared_ptr<IIonDevice> spIonDevice)
{
    MBOOL ret = MTRUE;

    uint32_t w = info.width;
    uint32_t h = info.height;
    uint32_t format = info.format;
    uint32_t aligned = info.aligned;
    bool isPixelAligned = info.isPixelAligned;

    size_t planeCount = queryPlaneCount(format);
    MUINT32 strideInBytes[3] = {0};
    for (size_t i = 0; i < planeCount; i++)
    {
        if (isPixelAligned) {
            MUINT32 pixelAlignedWidth = ALIGN_CEIL(w, aligned);
            strideInBytes[i] =
                (queryPlaneWidthInPixels(format, i, pixelAlignedWidth) *
                 queryPlaneBitsPerPixel(format, i) + 7) / 8;
        } else {
            // Bytes aligend
            strideInBytes[i] =
                (queryPlaneWidthInPixels(format, i, w) *
                 queryPlaneBitsPerPixel(format, i) + 7) / 8;
            strideInBytes[i] = ALIGN_CEIL(strideInBytes[i], aligned);
        }
        CAM_LOGD("allocBuffer strideInBytes[%zu] strideInBytes(%d)",
                i, strideInBytes[i]);
    }


    MINT32 bufBoundaryInBytes[3] = {0};
    IIonImageBufferHeap::AllocImgParam_t imgParam(
            format, MSize(w,h), strideInBytes, bufBoundaryInBytes, planeCount);
    IIonImageBufferHeap::AllocExtraParam rExtraParam;
    rExtraParam.ionDevice = spIonDevice;

    sp<IIonImageBufferHeap> heap =
        IIonImageBufferHeap::create(LOG_TAG,
                imgParam, rExtraParam, MFALSE);
    if (heap == nullptr) {
        CAM_LOGF("heap is nullptr");
        return MFALSE;
    }

    sp<IImageBuffer> pBuf = heap->createImageBuffer();
    if (pBuf == nullptr) {
        CAM_LOGF("pBuf is NULL");
        return MFALSE;
    }

    if (!pBuf->lockBuf(LOG_TAG, BUFFER_USAGE_SW)) {
        CAM_LOGF("lock Buffer failed");
        return MFALSE;
    }
    else
    {
        CAM_LOGD("allocBuffer addr(%p) size(%dx%d) format(0x%x)",
                pBuf.get(), w, h, format);
        String8 msg("allocBuffer");
        for (size_t i = 0; i < planeCount; i++)
        {
            msg.appendFormat(
                    " plane:va(%zu:%#" PRIxPTR ")", i, pBuf->getBufVA(i));
        }
        CAM_LOGD("%s", msg.string());

        imageBuf = pBuf;
    }

    return ret;
}

void ImageBufferUtils::deallocBuffer(IImageBuffer* pBuf)
{
    if (pBuf == NULL)
    {
        CAM_LOGD("pBuf is NULL, do nothing");
        return;
    }

    // unlock image buffer
    pBuf->unlockBuf(LOG_TAG);

    // unlock internal buffer
    {
        Mutex::Autolock _l(mInternalBufferListLock);
        const MINTPTR key = reinterpret_cast<MINTPTR>(pBuf);
        sp<IImageBuffer> tmpImageBuffer = mInternalBufferList.valueFor(key);
        if (tmpImageBuffer.get())
        {
            tmpImageBuffer->unlockBuf(LOG_TAG);
            mInternalBufferList.removeItem(key);
        }
    }
}

void ImageBufferUtils::deallocBuffer(sp<IImageBuffer>& pBuf)
{
    deallocBuffer(pBuf.get());
}

IImageBuffer* ImageBufferUtils::createBufferAlias(
        IImageBuffer* pOriginalBuf, MUINT32 w, MUINT32 h, EImageFormat format)
{
    MBOOL ret = MTRUE;

    MSize imgSize(w, h);
    size_t bufStridesInBytes[3] = {0};

    IImageBuffer *pBuf = NULL;
    IImageBufferHeap *pBufHeap = NULL;

    if (!pOriginalBuf)
    {
        CAM_LOGE("pOriginalBuf is null");
        ret = MFALSE;
        goto lbExit;
    }

    CAM_LOGD("pOriginalBuf is %p", pOriginalBuf);

    pBufHeap = pOriginalBuf->getImageBufferHeap();
    if (!pBufHeap)
    {
        CAM_LOGE("pBufHeap is null");
        ret = MFALSE;
        goto lbExit;
    }

    if (pBufHeap->getImgFormat() != eImgFmt_BLOB)
    {
        CAM_LOGE("heap buffer type must be BLOB=0x%x, this is 0x%x",
                eImgFmt_BLOB,
                pBufHeap->getImgFormat());
        ret = MFALSE;
        goto lbExit;
    }

    //@todo use those functions to replace this block
    //MINT32 const planeImgWidthStrideInPixels = Format::queryPlaneWidthInPixels(getImgFormat(), planeIndex, getImgSize().w);
    //MINT32 const planeImgHeightStrideInPixels= Format::queryPlaneHeightInPixels(getImgFormat(), planeIndex, getImgSize().h);
    #if 1
    switch (format)
    {
        case eImgFmt_Y8:
        case eImgFmt_JPEG:
            bufStridesInBytes[0] = w;
            break;
        case eImgFmt_I420:
            bufStridesInBytes[0] = w;
            bufStridesInBytes[1] = w / 2;
            bufStridesInBytes[2] = w / 2;
            break;
        case eImgFmt_YUY2:
            bufStridesInBytes[0] = w * 2;
            bufStridesInBytes[1] = w;
            bufStridesInBytes[2] = w;
            break;
        default:
            CAM_LOGE("unsupported format(0x%x)", format);
            ret = MFALSE;
            goto lbExit;
    }
    #else
    GetStride(w, format, bufStridesInBytes);
    #endif

    // create new buffer
    pBuf = pBufHeap->createImageBuffer_FromBlobHeap(
            (size_t)0, format, imgSize, bufStridesInBytes);

    if (!pBuf)
    {
        CAM_LOGE("can't makeBufferAlias size(%dx%d) format(0x%x)", w, h, format);
        ret = MFALSE;
        goto lbExit;
    }

    pBuf->incStrong(pBuf);

    // unlock old buffer
    if (!pOriginalBuf->unlockBuf(LOG_TAG))
    {
        CAM_LOGE("can't unlock pOriginalBuf");
    };

    // lock new buffer
    if (!pBuf->lockBuf(LOG_TAG, BUFFER_USAGE_SW))
    {
        CAM_LOGE("can't lock pBuf");
        ret = MFALSE;
        goto lbExit;
    }

lbExit:
    return pBuf;
}

MBOOL ImageBufferUtils::removeBufferAlias(
        IImageBuffer *pOriginalBuf, IImageBuffer *pAliasBuf)
{
    MBOOL ret = MTRUE;

    if ((pOriginalBuf == NULL) || (pAliasBuf == NULL))
    {
        CAM_LOGE("ImageBuffer is NULL: pOriginalBuf(%p) pAliasBuf(%p)",
                pOriginalBuf, pAliasBuf);
        ret = MFALSE;
        goto lbExit;
    }

    // destroy alias
    if (!pAliasBuf->unlockBuf(LOG_TAG))
    {
        CAM_LOGE("can't unlock pAliasBuf");
        ret = MFALSE;
        goto lbExit;
    }
    pAliasBuf->decStrong(pAliasBuf);

    // re-lock the original buffer
    if (!pOriginalBuf->lockBuf(LOG_TAG, BUFFER_USAGE_SW))
    {
        CAM_LOGE("can't lock pOriginalBuf");
        ret = MFALSE;
        goto lbExit;
    }

lbExit:
    return ret;
}

MBOOL ImageBufferUtils::createBuffer(sp<IImageBuffer> & output_buf, sp<IImageBuffer> & input_buf)
{
    MINT32 planeCount = input_buf->getPlaneCount();

    MINT32 format = eImgFmt_UNKNOWN;
    if (planeCount == 3) {
        format = *((MINT32*)input_buf->getBufVA(2));
    } else {
        format = input_buf->getImgFormat();
    }

    // create buffer
    MINT32 bufBoundaryInBytes[3] = {0, 0, 0};
    MUINT32 bufStridesInBytes[3] = {0, 0, 0};

    for (int i = 0 ;i < planeCount; ++i) {
        bufStridesInBytes[i] = input_buf->getBufStridesInBytes(i);
    }

    IImageBufferAllocator::ImgParam imgParam =
        IImageBufferAllocator::ImgParam((EImageFormat)format,
            input_buf->getImgSize(), bufStridesInBytes, bufBoundaryInBytes, (size_t)planeCount);
    sp<IIonImageBufferHeap> pHeap =
        IIonImageBufferHeap::create(LOG_TAG, imgParam);
    if (pHeap == NULL) {
        CAM_LOGE("[%s] Stuff ImageBufferHeap create fail", LOG_TAG);
        return MFALSE;
    }

    MINT imgFormat = pHeap->getImgFormat();
    ImgBufCreator creator(imgFormat);
    sp<IImageBuffer> pImgBuf = pHeap->createImageBuffer(&creator);
    if (pImgBuf == NULL) {
        CAM_LOGE("[%s] Stuff ImageBuffer create fail", LOG_TAG);
        return MFALSE;
    }

    // lock buffer
    if (!(pImgBuf->lockBuf(LOG_TAG, BUFFER_USAGE_SW))) {
        CAM_LOGE("[%s] Stuff ImageBuffer lock fail", LOG_TAG);
        return MFALSE;
    }

    output_buf = pImgBuf;
    return MTRUE;
}
