/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
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
#define LOG_TAG "MfllCore/ImageBuffer"

#include "MfllImageBuffer.h"

// MFNR Core Library
#include <mtkcam3/feature/mfnr/MfllLog.h>

// MTKCAM
#include <mtkcam/def/UITypes.h>
#include <mtkcam/def/ImageFormat.h>
#include <mtkcam/utils/imgbuf/ImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IDummyImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
#include <mtkcam/utils/std/Format.h>

// AOSP
#include <utils/Mutex.h> // android::Mutex
#include <utils/Condition.h>
#include <cutils/compiler.h>

// C
#include <stdio.h> // FILE

// STL
#include <functional> // std::function
#include <vector> // std::vector

#define BUFFER_USAGE    (eBUFFER_USAGE_SW_READ_OFTEN | \
                         eBUFFER_USAGE_SW_WRITE_OFTEN | \
                         eBUFFER_USAGE_HW_CAMERA_READWRITE)

namespace mfll{
template <class T>
inline android::sp<T> make_shared(T* var)
{
    return android::sp<T>(var);
}
}; // namespace mfll

#define ALIGN(w, a) (((w + (a-1)) / a) * a)

using namespace mfll;
using namespace NSCam;
using namespace NSCam::Utils::Format;
using android::sp;
using android::Mutex;
using android::Condition;
using NSCam::IImageBuffer;
/**
 *  Ring buffer queue for ZSD DualPhase useage {{{
 */
#if MFLL_USING_IMAGEBUFFER_QUEUE
namespace mfll {
#define MAX_BUFFER_COUNT MFLL_USING_IMAGEBUFFER_QUEUE
template <class T, class Deleter_T>
struct BufferQueue
{
    Mutex       mx;
    Condition   cond;
    std::vector<T> queue;
    Deleter_T deleter;
    int refCount; // counting buffer number in using

    BufferQueue(Deleter_T d) : refCount(0)
    {
        deleter = d;
    };

    T dequeBuffer()
    {
        Mutex::Autolock _l(&mx);
        T val = NULL;

        refCount++;

        /* no buffer */
        if (queue.size() <= 0) {
            /* check if achieve limit */
            if (refCount <= MAX_BUFFER_COUNT)
                return NULL;

            cond.wait(mx);
        }

        val = queue.front();
        queue.erase(queue.begin());
        return val;
    };

    void enqueBuffer(T &val)
    {
        Mutex::Autolock _l(&mx);

        refCount--;

        queue.push_back(val);

        /* the last one is back to queue, it's time to free all resource */
        if (refCount <= 0) {
            for (size_t i = 0; i < queue.size(); i++) {
                deleter(queue[i]);
            }
            queue.clear();
        }
        else
            cond.signal(); // unlock thread one-by-one
    };
};
}; // namespace mfll
static mfll::BufferQueue<MfllImageBuffer*, std::function<void(MfllImageBuffer*)>>
g_bufQueue_fullSize([](MfllImageBuffer*p)->void{
    p->handleByQueue(false);
    delete p;
});
#define DEQUEBUFFER()   g_bufQueue_fullSize.dequeBuffer()
#define ENQUEBUFFER(x)  g_bufQueue_fullSize.enqueBuffer(x)
#define ISASSOCIATEQBUFFER(x) (x == Flag_FullSize)
#else //MFLL_USING_IMAGEBUFFER_QUEUE
#define DEQUEBUFFER()   0
#define ENQUEBUFFER(x)  do {} while(0)
#define ISASSOCIATEQBUFFER(x) (false)
#endif//MFLL_USING_IMAGEBUFFER_QUEUE
/**
 * }}} Ring buffer queue
 */

IMfllImageBuffer* IMfllImageBuffer::createInstance(
        const char *bufferName /* = "" */,
        const IMfllImageBuffer_Flag_t &flag /* = Flag_Undefined */)
{
#if MFLL_USING_IMAGEBUFFER_QUEUE
    if (ISASSOCIATEQBUFFER(flag)) {
        MfllImageBuffer *p = DEQUEBUFFER();
        if (p == 0) {
            p = new MfllImageBuffer(bufferName);
            p->handleByQueue(true);
        }
        return (IMfllImageBuffer*)(p);
    }
    else
        return (IMfllImageBuffer*)new MfllImageBuffer(bufferName);
#else
    return (IMfllImageBuffer*)new MfllImageBuffer(bufferName);
#endif//MFLL_USING_IMAGEBUFFER_QUEUE
}

void IMfllImageBuffer::destroyInstance(void)
{
    decStrong((void*)this);
}

MfllImageBuffer::MfllImageBuffer(const char *bufferName /* = "" */)
{
    m_imgBuffer = NULL;
    m_imgHeap = NULL;
    m_imgChunk = NULL;
    m_spEventDispatcher = NULL;
    m_width = 0;
    m_height = 0;
    m_alignedWidth = 2;
    m_alignedHeight = 2;
    m_bufferSize = 0;
    m_bHasOwnership = true;
    m_bufferName = bufferName;
    m_handleByQueue = false;
    m_format = ImageFormat_Size;
    m_pCore = NULL;
}

MfllImageBuffer::~MfllImageBuffer(void)
{
    if (isHandleByQueue()) {
        MfllImageBuffer *pImg = new MfllImageBuffer;
        *pImg = *this;
        ENQUEBUFFER(pImg);
    }
    else {
        releaseBuffer();
    }
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

static bool is_yuv422(ImageFormat f)
{
    return
        (f == ImageFormat_Yuy2) ||
        (f == ImageFormat_Yv16) ||
        (f == ImageFormat_I422) ;
}

enum MfllErr MfllImageBuffer::setResolution(unsigned int w, unsigned int h)
{
    enum MfllErr err = MfllErr_Ok;
    m_width = w;
    m_height = h;
lbExit:
    return err;
}

enum MfllErr MfllImageBuffer::setAligned(unsigned int aligned_w, unsigned int aligned_h)
{
    enum MfllErr err = MfllErr_Ok;
    m_alignedWidth = aligned_w;
    m_alignedHeight = aligned_h;
lbExit:
    return err;
}

enum MfllErr MfllImageBuffer::setImageFormat(enum ImageFormat f)
{
    enum MfllErr err = MfllErr_Ok;

    RWLock::AutoWLock _l(m_mutex);

    /* If not created by MfllImageBuffer, we cannot convert image format */
    if (m_bHasOwnership == false) {
        err = MfllErr_NotSupported;
        goto lbExit;
    }

    if (m_imgBuffer) {
        mfllLogW("%s: cannot set image format due to inited", __FUNCTION__);
        err = MfllErr_NotSupported;
        goto lbExit;
    }

    m_format = f;

lbExit:
    return err;
}

enum MfllErr MfllImageBuffer::getResolution(unsigned int &w, unsigned int &h)
{
    RWLock::AutoRLock _l(m_mutex);
    w = m_width;
    h = m_height;
    return MfllErr_Ok;
}

unsigned int MfllImageBuffer::getAlignedWidth(void) const
{
    return ALIGN(m_width, m_alignedWidth);
}

unsigned int MfllImageBuffer::getAlignedHeight(void) const
{
    return ALIGN(m_height, m_alignedHeight);
}

enum MfllErr MfllImageBuffer::getAligned(unsigned int &w, unsigned int &h)
{
    RWLock::AutoRLock _l(m_mutex);
    w = m_alignedWidth;
    h = m_alignedHeight;
    return MfllErr_Ok;
}

enum ImageFormat MfllImageBuffer::getImageFormat(void)
{
    RWLock::AutoRLock _l(m_mutex);
    return m_format;
}

enum MfllErr MfllImageBuffer::initBuffer(void)
{
    enum MfllErr err = MfllErr_Ok;
    mfllFunctionIn();

    RWLock::AutoWLock _l(m_mutex);

    if (m_imgBuffer) {
        err = MfllErr_Shooted;
        goto lbExit;
    }

    {
        /* create buffer */
        unsigned int imageSize = 0;
        err = createContinuousImageBuffer(m_format, imageSize);
        if (err == MfllErr_Ok)
            m_bufferSize = imageSize;
    }

lbExit:
    if (err != MfllErr_Ok) {
        if (err == MfllErr_Shooted) {
            mfllLogW("%s: Image buffer has been inited", __FUNCTION__);
            err = MfllErr_Ok; // set MfllErr_Ok.
        }
        else
            mfllLogE("%s: create buffer failed with code %d", __FUNCTION__, (int)err);
    }

    mfllFunctionOut();
    return err;
}

bool MfllImageBuffer::isInited(void)
{
    RWLock::AutoRLock _l(m_mutex);
    return m_imgBuffer ? true : false;
}

enum MfllErr MfllImageBuffer::convertImageFormat(const enum ImageFormat &f)
{
    enum MfllErr err = MfllErr_Ok;

    RWLock::AutoWLock _l(m_mutex);

    if (!m_bHasOwnership) {
        mfllLogE("%s: don't own the ownership", __FUNCTION__);
        err = MfllErr_NotSupported;
        goto lbExit;
    }

    if (m_imgBuffer == NULL) {
        mfllLogE("%s: image buffer hasn't been inited", __FUNCTION__);
        err = MfllErr_NullPointer;
        goto lbExit;
    }

    err = convertImageBufferFormat(f);
    if (err != MfllErr_Ok) {
        mfllLogE("%s: convert image buffer format failed", __FUNCTION__);
    }

    m_format = f;

lbExit:
    return err;
}

enum MfllErr MfllImageBuffer::setImageBuffer(void *lpBuffer)
{
    enum MfllErr err = MfllErr_Ok;
    IImageBuffer *pBuffer = NULL;

    if (__builtin_expect( lpBuffer == nullptr, false )) {
        mfllLogE("%s: NULL argument", __FUNCTION__);
        return MfllErr_BadArgument;
    }

    RWLock::AutoRLock _l(m_mutex);

    if (m_imgBuffer) {
        err = MfllErr_Shooted;
        goto lbExit;
    }

    pBuffer = reinterpret_cast<IImageBuffer*>(lpBuffer);
    pBuffer->incStrong((void*)pBuffer);

    m_bHasOwnership = false; // Don't have ownership
    m_imgOwnFromOther = pBuffer;
    m_imgBuffer = pBuffer;
    m_width = pBuffer->getImgSize().w;
    m_height = pBuffer->getImgSize().h;
    m_alignedWidth = 1;
    m_alignedHeight = 1;

    /* get image buffer size */
    m_bufferSize = 0;
    for (MUINT32 i = 0; i < pBuffer->getPlaneCount(); i++)
        m_bufferSize += pBuffer->getBufSizeInBytes(i);

    m_format = mapImageFormat(pBuffer->getImgFormat());

lbExit:

    if (err != MfllErr_Ok) {
        if (err == MfllErr_Shooted) {
            mfllLogW("%s: Image buffer has been inited", __FUNCTION__);
            err = MfllErr_Ok; // set MfllErr_Ok.
        }
        else
            mfllLogE("%s: create buffer failed with code %d", __FUNCTION__, (int)err);
    }

    return MfllErr_Ok;
}

void* MfllImageBuffer::getVa(void)
{
    void *va = NULL;

    RWLock::AutoRLock _l(m_mutex);

    if (m_imgBuffer) {
        va = (void*)(long)m_imgBuffer->getBufVA(0);
    }
    return va;
}

size_t MfllImageBuffer::getRealBufferSize(void)
{
    size_t r = 0;

    RWLock::AutoRLock _l(m_mutex);

    if (m_imgBuffer)
        r = m_bufferSize;

    return r;
}

size_t MfllImageBuffer::getBufStridesInBytes(size_t index)
{
    RWLock::AutoRLock _l(m_mutex);
    if (CC_LIKELY(m_imgBuffer != nullptr)) {
        return m_imgBuffer->getBufStridesInBytes(index);
    }
    return 0;
}

void* MfllImageBuffer::getImageBuffer(void)
{
    IImageBuffer *addr = NULL;
    RWLock::AutoRLock _l(m_mutex);

    if (m_imgBuffer)
        addr = m_imgBuffer;

    return reinterpret_cast<void*>(addr);
}

enum MfllErr MfllImageBuffer::releaseBuffer(void)
{
    enum MfllErr err = MfllErr_Ok;

    mfllFunctionIn();

    RWLock::AutoWLock _l(m_mutex);

    releaseBufferNoLock();

lbExit:
    if (err == MfllErr_NullPointer) {
        mfllLogW("%s: m_imgBuffer is already NULL", __FUNCTION__);
        err = MfllErr_Ok; // set back ok!
    }

    mfllFunctionOut();
    return MfllErr_Ok;
}

enum MfllErr MfllImageBuffer::syncCache(void)
{
    RWLock::AutoWLock _l(m_mutex);
    if (CC_LIKELY(m_imgBuffer != nullptr)) {
        m_imgBuffer->syncCache();
        return MfllErr_Ok;
    }
    return MfllErr_NullPointer;
}

void MfllImageBuffer::operator=(const MfllImageBuffer &in)
{
    m_width                 = in.m_width;
    m_height                = in.m_height;
    m_alignedWidth          = in.m_alignedWidth;
    m_alignedHeight         = in.m_alignedHeight;
    m_bufferSize            = in.m_bufferSize;
    m_format                = in.m_format;
    m_spEventDispatcher     = in.m_spEventDispatcher;
    m_pCore                 = in.m_pCore;
    m_imgBuffer             = in.m_imgBuffer;
    m_imgHeap               = in.m_imgHeap;
    m_imgChunk              = in.m_imgChunk;
    m_imgOwnFromOther       = in.m_imgOwnFromOther;
    m_bHasOwnership         = in.m_bHasOwnership;
    m_bufferName            = in.m_bufferName;
    m_handleByQueue         = in.m_handleByQueue;
}

enum MfllErr MfllImageBuffer::registerEventDispatcher(const sp<IMfllEvents> &e)
{
    RWLock::AutoWLock _l(m_mutex);
    m_spEventDispatcher = e;
    return MfllErr_Ok;
}

enum MfllErr MfllImageBuffer::saveFile(const char *name, bool commonDump)
{
    enum MfllErr err = MfllErr_Ok;
    FILE *fp = NULL;
    size_t fileSize = 0;
    size_t resultSize = 0;
    size_t bufferSize = 0;

    mfllFunctionIn();

    RWLock::AutoWLock _l(m_mutex);

    if (m_imgBuffer == NULL) {
        err = MfllErr_NullPointer;
        goto lbExit;
    }

    /* check has ownwership or not */
    if (m_bHasOwnership && !commonDump) {
        /* check file */
        fp = fopen(name, "wb");
        if (fp == NULL) {
            err = MfllErr_BadArgument;
            goto lbExit;
        }

        /* get buffer size */
        bufferSize = getRealBufferSize();

        /* save file */
        void *addr = (void*)(long)m_imgBuffer->getBufVA(0);
        mfllLogD("%s: write file with size %zu to %s", __FUNCTION__, bufferSize, name);
        fwrite(addr, 1L, bufferSize, fp); // write to file.
    }
    else {
        mfllLogD("%s: write file using IImageBuffer::saveToFile due to no ownership", __FUNCTION__);
        mfllLogD("%s: save file to %s", __FUNCTION__, name);
        /* w/o ownership, using IImageBuffer::saveToFile instead */
        m_imgBuffer->saveToFile(name);
    }

lbExit:
    if (fp)
        fclose(fp);

    if (err == MfllErr_NullPointer) {
        mfllLogE("%s: can't save file with NULL pointer", __FUNCTION__);
    }
    else if (err == MfllErr_UnexpectedError) {
        mfllLogE("%s: save file failed with unexpected fail", __FUNCTION__);
    }

    mfllFunctionOut();

    return err;
}

enum MfllErr MfllImageBuffer::loadFile(const char *name)
{
    enum MfllErr err = MfllErr_Ok;
    MBOOL bRet = MTRUE;
    FILE *fp = NULL;
    size_t fileSize = 0;
    size_t resultSize = 0;
    size_t bufferSize = 0;

    RWLock::AutoWLock _l(m_mutex);

    /* check imgBuf */
    if (m_imgBuffer == NULL) {
        err = MfllErr_NullPointer;
        goto lbExit;
    }

    /* check file */
    fp = fopen(name, "rb");
    if (fp == NULL) {
        err = MfllErr_BadArgument;
        goto lbExit;
    }

    /* tell file size */
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    bufferSize = getRealBufferSize();

    /* check file size with aligned */
    if (bufferSize != fileSize) {
        err = MfllErr_BadArgument;
        goto lbExit;
    }
    {
        void *addr = (void*)(long)m_imgBuffer->getBufVA(0);
        fread(addr, 1L, bufferSize, fp);
    }

lbExit:
    if (fp)
        fclose(fp);

    if (err == MfllErr_NullPointer) {
        mfllLogE("%s: load file failed with NULL image buffer", __FUNCTION__);
    }
    else if (err == MfllErr_BadArgument) {
        if (bufferSize != fileSize)
            mfllLogE("%s: file size doesn't match, bufferSize = %zu, fileSize = %zu", __FUNCTION__, bufferSize, fileSize);
        else
            mfllLogE("%s: file %s cannot be opened", __FUNCTION__, name);
    }

    return err;
}

static void calculate_stride_offset_size(
    unsigned int width,
    unsigned int height,
    unsigned int alignedw,
    unsigned int alignedh,
    ImageFormat f,
    MUINT32 strideInBytes[], // size should be 3
    MUINT32 addrOffset[], // size should be 3
    MUINT32 &imageSize
)
{
    /* calculate total size, stride, and address of each planes */
    MUINT32 imgFmt = MfllImageBuffer::mapImageFormat(f);
    MUINT32 planeCount = queryPlaneCount(imgFmt);
    imageSize = 0; // in bytes
    for (MUINT32 i = 0; i < planeCount; i++) {
        unsigned int w = queryPlaneWidthInPixels(imgFmt, i, alignedw);
        unsigned int h = queryPlaneHeightInPixels(imgFmt, i, alignedh);
        unsigned int bitsPerPixel = queryPlaneBitsPerPixel(imgFmt, i);

        strideInBytes[i] = (w * bitsPerPixel) / 8;
        addrOffset[i] = imageSize;

        imageSize += (w * h * bitsPerPixel) / 8; // calcuate in bytes

        mfllLogD("%s: plane %d, stride = %d, addr_offset = %d", __FUNCTION__, i, strideInBytes[i], addrOffset[i]);
    }

    mfllLogD("%s: create buffer with (w,h,alignedw,alignedh,fmt)=(%d,%d,%d,%d,%d)", __FUNCTION__, width, height, alignedw, alignedh, (int)imgFmt);
    mfllLogD("%s: buffer size = %d", __FUNCTION__, imageSize);

}

/**
 *  create alias image buffer, notice that, we need to release IImageBufferHeap,
 *  or it cause fd leak.
 *
 *  @param width            Width of the original image buffer, in pixel
 *  @param height           Height of the original image buffer, in pixel
 *  @param alginedw         Aligned width of the image buffer you wanna create, in pixel
 *  @param alignedh         Aligned height of the image buffer you wanna create, in pixel
 *  @param f                Image format you wanna create
 *  @param startAddrVa      The start address of the first image plane, if your image
 *                          format which has more than a plane, the second and the following
 *                          plane address will be startAddr + addrOffset[n]
 *  @param startAddrPa      Familiar with @param startAddrVa, but it's physical address of
 *                          the IImageBuffer, use IImageBuffer::getBufPA to get the first PA
 *  @param fd               The ION fd of the blob image buffer
 *  @param strideInByte[]   Describe stride in bytes for the planes
 *  @param addrOffset[]     Address offset for every plane, the offset of the first
 *                          plane should be 0.
 *  @param[out] ppHeap      IImageBufferHeap of the IImageBuffer, caller MUST release
 *                          this IImageBufferHeap after used
 */
static IImageBuffer* encapsulate_image_buffer(
    unsigned int width,
    unsigned int height,
    ImageFormat f,
    MUINTPTR startAddrVa,
    MUINTPTR startAddrPa,
    MINT32 fd,
    MUINT32 strideInBytes[], // size should be 3
    MUINT32 addrOffset[], // size should be 3
    IImageBufferHeap **ppHeap // heap that we have to manage it
)
{
    IImageBufferAllocator *allocator = NULL;
    IImageBufferHeap *pHeap = NULL;
    IImageBuffer *pBuf = NULL;
    MUINT32 imgFmt = MfllImageBuffer::mapImageFormat(f);
    MUINT32 planeCount = queryPlaneCount(imgFmt);
    MINT32 bufBoundaryInBytes[3] = {0}; // always be zero

    /* declare IImageBuffer information */
    IImageBufferAllocator::ImgParam extParam(imgFmt, MSize(width, height), strideInBytes, bufBoundaryInBytes, planeCount);

    MINT32      _memID[3];
    MUINTPTR    _virtAddr[3];
    MUINTPTR    _phyAddr[3];
    for (int i = 0; i < 3; i++) {
        _memID[i] = fd;
        _virtAddr[i] = startAddrVa + addrOffset[i];
        _phyAddr[i] = startAddrPa + addrOffset[i];
    }

    if (planeCount > 1) {
        /**
         *  non-continuous buffer, we need to specify both VA and PA, hence we only
         *  can use IDummyImageBufferHeap to create a all customized one
         */
        PortBufInfo_dummy portBufInfo = PortBufInfo_dummy(_memID[0], _virtAddr, _phyAddr, planeCount);
        portBufInfo.offset = addrOffset[0];
        pHeap = IDummyImageBufferHeap::create(MFLL_LOG_KEYWORD, extParam, portBufInfo);
    }
    else {
        /* A plane */
        PortBufInfo_v1 portBufInfo = PortBufInfo_v1(_memID[0], _virtAddr[0]);
        portBufInfo.offset[0] = addrOffset[0];
        pHeap = ImageBufferHeap::create(MFLL_LOG_KEYWORD, extParam, portBufInfo);
    }

    if (pHeap == NULL) {
        mfllLogE("%s: create heap of blob failed", __FUNCTION__);
        goto lbExit;
    }

    /* release heap first */
    if (*ppHeap) {
        mfll::make_shared<IImageBufferHeap>(*ppHeap);
        *ppHeap = NULL;
    }
    *ppHeap = pHeap;

    /* create image buffer */
    pBuf = pHeap->createImageBuffer();
    if (pBuf == NULL) {
        mfll::make_shared<IImageBufferHeap>(pHeap);
        mfllLogE("%s: create image buffer from blob heap fail", __FUNCTION__);
        goto lbExit;
    }
    /* Great! You got an image buffer */

lbExit:
    return pBuf;
}

/**
 *  The idea to create a continuous image bufer is:
 */
enum MfllErr MfllImageBuffer::createContinuousImageBuffer(const enum ImageFormat& f, unsigned int &imageSize)
{
    enum MfllErr err = MfllErr_Ok;
    MUINT32 imgFmt = mapImageFormat(f);
    MUINT32 planeCount = queryPlaneCount(imgFmt);
    MINT32 bufBoundaryInBytes[3] = {0}; // always be zero
    MUINT32 strideInBytes[3] = {0};
    MUINT32 addrOffset[3] = {0};
    MUINT32 width, height, alignedw, alignedh;
    MUINT32 bpp = 0;
    imageSize = 0;
    void *bufferVa = NULL;
    void *bufferPa = NULL;
    int fd = 0;

    /* check image format if valid*/
    if (imgFmt == 0) {
        mfllLogE("%s: Invalid input image format=%d", __FUNCTION__, (int)f);
        return MfllErr_BadArgument;
    }

    width = m_width;
    height = m_height;
    alignedw = m_alignedWidth;
    alignedh = m_alignedHeight;

    /* algin width and height */
    alignedw = ALIGN(width, alignedw);
    alignedh = ALIGN(height, alignedh);

    /* calculate total size, stride, and address of each planes */
    calculate_stride_offset_size(width, height, alignedw, alignedh, f, strideInBytes, addrOffset, imageSize);

    /* create continuous blob heap and get virtual address */
    {
        sp<IImageBuffer> spTemp = NULL;
        sp<IImageBufferHeap> pHeap = NULL;
        IImageBufferAllocator::ImgParam imgParam(imageSize, 0); // blob buffer.

        pHeap = IIonImageBufferHeap::create(MFLL_LOG_KEYWORD, imgParam);
        if (pHeap == NULL) {
            mfllLogF("%s: create blob buffer heap fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        spTemp = pHeap->createImageBuffer();
        spTemp->lockBuf(MFLL_LOG_KEYWORD, BUFFER_USAGE);
        // spTemp->syncCache(eCACHECTRL_INVALID); // hw->cpu
        bufferVa = (void*)(long long)spTemp->getBufVA(0); // get buffer VA1
        bufferPa = (void*)(long long)spTemp->getBufPA(0); // get buffer PA
        fd = spTemp->getFD(); // FD
        m_imgChunk = spTemp;

    }

    /* encapsulate tempBuf into external IImageBuffer */
    {
        IImageBuffer *pBuf = encapsulate_image_buffer(
                width, height, // image resolution
                f, // image format
                (MUINTPTR)(long long)bufferVa, // the start address (VA)
                (MUINTPTR)(long long)bufferPa,
                fd, // ION fd
                strideInBytes, // strideInBytes[3]
                addrOffset, // addrOffset[3]
                &m_imgHeap); // output heap

        if (pBuf == NULL) {
            mfllLogE("%s: create image buffer from blob heap fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        /* Great! You got an image buffer */
        m_imgBuffer = pBuf; // hold the buffer
    }

    if (!m_imgBuffer->lockBuf(MFLL_LOG_KEYWORD, BUFFER_USAGE)) {
        mfllLogE("%s: lock buffer fail", __FUNCTION__);
        goto lbExit;
    }

    mfllLogD("%s: allocated buffer %p with size %d", __FUNCTION__, (void*)m_imgBuffer, imageSize);

lbExit:

    return err;
}

enum MfllErr MfllImageBuffer::convertImageBufferFormat(const enum ImageFormat &f)
{
    enum MfllErr err = MfllErr_Ok;
    MBOOL ret = MTRUE;
    MUINT32 imgFmt = mapImageFormat(f);
    MUINT32 planeCount = queryPlaneCount(imgFmt);
    MINT32 bufBoundaryInBytes[3] = {0}; // always be zero
    MUINT32 strideInBytes[3] = {0};
    MUINT32 addrOffset[3] = {0};
    MUINT32 width, height, alignedw, alignedh;
    MUINT32 bpp = 0;
    MUINT32 imageSize = 0;
    void *bufferVa = NULL;
    void *bufferPa = NULL;
    int fd = 0;

    if (m_imgBuffer == NULL) {
        mfllLogE("%s: m_imgBuffer is NULL", __FUNCTION__);
        err = MfllErr_NullPointer;
        goto lbExit;
    }

    /* only supports YUV422 conversion */
    if (!is_yuv422(f)) {
        mfllLogE("%s: Unsupport image format", __FUNCTION__);
        err = MfllErr_BadArgument;
        goto lbExit;
    }

    /* only supports YUV422 conversion */
    if (!is_yuv422(m_format)) {
        mfllLogE("%s: The buffer format cannot be converted", __FUNCTION__);
        err = MfllErr_NotSupported;
        goto lbExit;
    }

    /* for thread-safe */
    width = m_width;
    height = m_height;
    alignedw = m_alignedWidth;
    alignedh = m_alignedHeight;

    /* algin width and height */
    alignedw = ALIGN(width, alignedw);
    alignedh = ALIGN(height, alignedh);

    /* calculate total size, stride, and address of each planes */
    calculate_stride_offset_size(width, height, alignedw, alignedh, f, strideInBytes, addrOffset, imageSize);

    /* encapsulate tempBuf into external IImageBuffer */
    {
        IImageBufferHeap *pHeap = NULL;
        IImageBuffer *pBuf = encapsulate_image_buffer(
                width, height,
                f,
                m_imgBuffer->getBufVA(0),
                m_imgBuffer->getBufPA(0),
                m_imgBuffer->getFD(),
                strideInBytes,
                addrOffset,
                &pHeap);

        if (pBuf == NULL) {
            mfllLogE("%s: create image buffer from blob heap fail", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        /* release old image buffer and heap */
        do {
            // tricky: we have to use android::sp to release IImageBufferHeap and
            //         IImageBuffer due to forbidden destructor. And, we MUST
            //         release IImageBufferHeap first because in
            //         android::RefBase::onLastStrongRef of IImageBuffer, the
            //         IImageBufferHeap will be released again by using android::sp.
            mfll::make_shared<IImageBufferHeap> ( m_imgHeap     );
            mfll::make_shared<IImageBuffer>     ( m_imgBuffer   );
            m_imgBuffer = nullptr;
            m_imgHeap = nullptr;
        } while (0);

        /* Great! You get a new image buffer and heap */
        m_imgBuffer = pBuf;
        m_imgHeap   = pHeap;
    }

    if (!m_imgBuffer->lockBuf(MFLL_LOG_KEYWORD, BUFFER_USAGE)) {
        mfllLogE("%s: lock buffer fail", __FUNCTION__);
        goto lbExit;
    }

    mfllLogD("%s: allocated buffer %p with size %d", __FUNCTION__, (void*)m_imgBuffer, imageSize);

lbExit:

    return err;
}

enum MfllErr MfllImageBuffer::releaseBufferNoLock(void)
{
    /* If MfllImageBuffer has ownwership, release IImageBuffer here */
    if (m_bHasOwnership) {
        if (m_imgBuffer) {
            m_imgBuffer->unlockBuf(MFLL_LOG_KEYWORD);
            /* release heap before buffer because m_imgHeap is not maintained by
             * android::sp and it may be duplicated released.*/
            sp<IImageBufferHeap> spHeap = m_imgHeap;
            spHeap = 0;
            /* release strong reference count using sp template */
            sp<IImageBuffer> sp = m_imgBuffer;
        }
        if (m_imgChunk.get()) {
            m_imgChunk->unlockBuf(MFLL_LOG_KEYWORD);
        }
    }
    else {
        if (m_imgBuffer) {
            m_imgBuffer->decStrong((void*)m_imgBuffer);
        }
    }


    m_bufferSize = 0;
    m_imgOwnFromOther = NULL;
    m_imgBuffer = NULL;
    m_imgChunk = NULL;
    m_imgHeap = NULL;
    m_bHasOwnership = true;

    return MfllErr_Ok;
}

MUINT32 MfllImageBuffer::mapImageFormat(const enum ImageFormat &fmt, bool dontCare /* = false */)
{
    MUINT32 f = 0;
    switch(fmt) {
    case ImageFormat_Yuy2:
        f = eImgFmt_YUY2;
        break;
    case ImageFormat_Yv16:
        f = eImgFmt_YV16;
        break;
    case ImageFormat_I422:
        f = eImgFmt_I422;
        break;
    case ImageFormat_Raw12:
        f = eImgFmt_BAYER12;
        break;
    case ImageFormat_Raw10:
        f = eImgFmt_BAYER10;
        break;
    case ImageFormat_Raw8:
        f = eImgFmt_BAYER8;
        break;
    case ImageFormat_Y8:
        f = eImgFmt_Y8; /* using bayer8 instead */
        break;
    case ImageFormat_Nv21:
        f = eImgFmt_NV21;
        break;
    case ImageFormat_Nv12:
        f = eImgFmt_NV12;
        break;
    default:
        f = 0xFFFFFFFF;
        if ( !dontCare )
            mfllLogE("%s: Unsupport format 0x%x", __FUNCTION__, fmt);
        break;
    }
    return f;
}

enum ImageFormat MfllImageBuffer::mapImageFormat(const MUINT32 &fmt, bool dontCare /* = false */)
{
    enum ImageFormat f;
    switch(fmt) {
    case eImgFmt_YUY2:
        f = ImageFormat_Yuy2;
        break;
    case eImgFmt_YV16:
        f = ImageFormat_Yv16;
        break;
    case eImgFmt_I422:
        f = ImageFormat_I422;
        break;
    case eImgFmt_BAYER12:
        f = ImageFormat_Raw12;
        break;
    case eImgFmt_BAYER10:
        f = ImageFormat_Raw10;
        break;
    case eImgFmt_BAYER8:
        f = ImageFormat_Raw8;
        break;
    case eImgFmt_Y8:
        f = ImageFormat_Y8;
        break;
    case eImgFmt_NV21:
        f = ImageFormat_Nv21;
        break;
    case eImgFmt_NV12:
        f = ImageFormat_Nv12;
        break;
    default:
        f = ImageFormat_Size;
        if ( !dontCare )
            mfllLogE("%s: Unsupport format 0x%x", __FUNCTION__, fmt);
        break;
    }
    return f;
}

unsigned int MfllImageBuffer::tranImageFormat(const enum ImageFormat &fmt)
{
    return mapImageFormat(fmt);
}

enum ImageFormat MfllImageBuffer::tranImageFormat(const unsigned int &fmt)
{
    return mapImageFormat(fmt);
}

void MfllImageBuffer::dumpDebugLog(const char* name)
{
    if (m_imgBuffer == nullptr)
        return;

    mfllLogD("%s: getImgFormat() = %x, getImgSize().w = %d, getImgSize().h = %d, getPlaneCount() = %d"
        , name
        , m_imgBuffer->getImgFormat()
        , m_imgBuffer->getImgSize().w
        , m_imgBuffer->getImgSize().h
        , m_imgBuffer->getPlaneCount()
    );

    for (int i = 0 ; i < m_imgBuffer->getPlaneCount() ; i++) {
        mfllLogD("%s: getBufStridesInBytes(%d) = %d, getBufSizeInBytes(%d) = %d, getBufVA(%d) = %p, getBufPA(%d) = %p"
            , name
            , i, m_imgBuffer->getBufStridesInBytes(i)
            , i, m_imgBuffer->getBufSizeInBytes(i)
            , i, m_imgBuffer->getBufVA(i)
            , i, m_imgBuffer->getBufPA(i)
        );
    }
}
