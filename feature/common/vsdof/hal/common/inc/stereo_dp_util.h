/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef _STEREO_DP_UTIL_H_
#define _STEREO_DP_UTIL_H_

#include <mtkcam/utils/std/Format.h>
#include <DpIspStream.h>
#include <android/log.h>

#include <mtkcam/utils/imgbuf/IGrallocImageBufferHeap.h>
#include <ui/gralloc_extra.h>
#include <vsdof/hal/ProfileUtil.h>

#include <vndk/hardware_buffer.h>    //For AHardwareBuffer

#define STEREO_DP_UTIL_TAG "StereoDpUtil"

using namespace NSCam;
#define IS_ALLOC_GB    true

#define CHECK_DP_SUCCESS(opStr, status)\
if(status != DP_STATUS_RETURN_SUCCESS)\
{\
    __android_log_print(ANDROID_LOG_ERROR, STEREO_DP_UTIL_TAG, "Operation failed: " #opStr);\
    return false;\
}

class StereoDpUtil {
public:
    //Notice: caller should create dst image with rotated size
    static bool transformImage(
                        IImageBuffer *imgSrc,
                        IImageBuffer *imgDst,
                        int eRotateDegree = 0,
                        DpRect *srcROI = NULL,
                        DpRect *dstROI = NULL
                       )
    {
        if(NULL == imgSrc ||
           NULL == imgDst)
        {
            return false;
        }

        DpIspStream   dpStream(DpIspStream::ISP_ZSD_STREAM);

        __setSrc(dpStream, imgSrc, srcROI);
        __setDst(dpStream, imgDst, eRotateDegree, dstROI);

        auto status = dpStream.startStream();
        CHECK_DP_SUCCESS(startStream, status);

        status = dpStream.dequeueSrcBuffer();
        CHECK_DP_SUCCESS(dequeueSrcBuffer, status);

        MINTPTR dstVAs[3] = {0, 0, 0};
        status = dpStream.dequeueDstBuffer(0, (void**)dstVAs, true);
        CHECK_DP_SUCCESS(dequeueDstBuffer, status);

        status = dpStream.dequeueFrameEnd();
        CHECK_DP_SUCCESS(dequeueFrameEnd, status);

        status = dpStream.stopStream();
        CHECK_DP_SUCCESS(stopStream, status);

        return true;
    }

    static bool transformImage(
                        IImageBuffer *imgSrc,
                        IImageBuffer *imgDst1,
                        IImageBuffer *imgDst2,
                        DpRect *srcROI = NULL,
                        int eRotateDst1=0,
                        DpRect *dstROI1 = NULL,
                        int eRotateDst2=0,
                        DpRect *dstROI2 = NULL
                       )
    {
        if(NULL == imgSrc ||
           NULL == imgDst1 ||
           NULL == imgDst2)
        {
            return false;
        }

        DpIspStream   dpStream(DpIspStream::ISP_ZSD_STREAM);

        __setSrc(dpStream, imgSrc, srcROI);
        __setDst(dpStream, imgDst1, eRotateDst1, dstROI1, 0);
        __setDst(dpStream, imgDst2, eRotateDst2, dstROI2, 1);

        auto status = dpStream.startStream();
        CHECK_DP_SUCCESS(startStream, status);

        status = dpStream.dequeueSrcBuffer();
        CHECK_DP_SUCCESS(dequeueSrcBuffer, status);

        MINTPTR dstVAs[2][3] = {{0, 0, 0}, {0, 0, 0}};
        status = dpStream.dequeueDstBuffer(0, (void**)dstVAs, true);
        for(int portIndex = 0; portIndex < 2; portIndex++)
        {
            status = dpStream.dequeueDstBuffer(portIndex, (void**)dstVAs[portIndex], true);
            CHECK_DP_SUCCESS(dequeueDstBuffer, status);
        }
        CHECK_DP_SUCCESS(dequeueDstBuffer, status);

        status = dpStream.dequeueFrameEnd();
        CHECK_DP_SUCCESS(dequeueFrameEnd, status);

        status = dpStream.stopStream();
        CHECK_DP_SUCCESS(stopStream, status);

        return true;
    }

    //Create IImgeBuffer from buffer, only support YV12 and mask
    static bool allocImageBuffer(const char *TAG, EImageFormat fmt, MSize size, bool isGB, sp<IImageBuffer>& retImage)
    {
        if( NULL == TAG
            || 0 == strlen(TAG)
            || ( eImgFmt_YV12     != fmt &&
                 eImgFmt_I420     != fmt &&
                 eImgFmt_Y8       != fmt &&
                 eImgFmt_RGBA8888 != fmt) )
        {
            return false;
        }

        IImageBufferAllocator* allocator = IImageBufferAllocator::getInstance();
        if( NULL == allocator ) {
            __android_log_write(ANDROID_LOG_ERROR, TAG, "cannot get allocator");
            return false;
        }

        MUINT const PLANE_COUNT = NSCam::Utils::Format::queryPlaneCount(fmt);
        MINT32 bufBoundaryInBytes[] = {0, 0, 0};
        MUINT32 bufStridesInBytes[] = {0, 0, 0};

        for (MUINT i = 0; i < PLANE_COUNT; i++)
        {
            bufStridesInBytes[i] =
                (NSCam::Utils::Format::queryPlaneWidthInPixels(fmt, i, size.w) * NSCam::Utils::Format::queryPlaneBitsPerPixel(fmt, i))>>3;
        }

        if(StereoSettingProvider::isLogEnabled()) {
            switch(fmt) {
                case eImgFmt_Y8:
                    __android_log_print(ANDROID_LOG_DEBUG, TAG, "alloc %d x %d, fmt 0x%X(eImgFmt_Y8)", size.w, size.h, fmt);
                    break;
                case eImgFmt_RGBA8888:
                    __android_log_print(ANDROID_LOG_DEBUG, TAG, "alloc %d x %d, fmt 0x%X(eImgFmt_RGBA8888)", size.w, size.h, fmt);
                    break;
                case eImgFmt_I420:
                    __android_log_print(ANDROID_LOG_DEBUG, TAG, "alloc %d x %d, fmt 0x%X(eImgFmt_I420)", size.w, size.h, fmt);
                    break;
                case eImgFmt_YV12:
                default:
                    __android_log_print(ANDROID_LOG_DEBUG, TAG, "alloc %d x %d, fmt 0x%X(eImgFmt_YV12)", size.w, size.h, fmt);
                    break;
            }
        }

        IImageBufferAllocator::ImgParam imgParam(
                fmt,
                size,
                bufStridesInBytes,
                bufBoundaryInBytes,
                PLANE_COUNT
                );

        //
        MUINT32 gbusage = eBUFFER_USAGE_HW_RENDER|eBUFFER_USAGE_HW_TEXTURE|eBUFFER_USAGE_SW_READ_OFTEN|eBUFFER_USAGE_SW_WRITE_OFTEN;
        if(isGB)
        {
            IImageBufferAllocator::ExtraParam extParam(gbusage);
            retImage = allocator->alloc_gb(TAG, imgParam, extParam);

            if( NULL == retImage.get() ) {
                return false;
            }

            // Enable GPU full mode
            IImageBufferHeap* pDstHeap  = retImage->getImageBufferHeap();
            if( NULL == pDstHeap ) {
                return false;
            }

            AHardwareBuffer* gbuf = (AHardwareBuffer*)(pDstHeap->getHWBuffer());
            if( NULL == gbuf ) {
                return false;
            }
            buffer_handle_t handle = AHardwareBuffer_getNativeHandle(gbuf);

            gralloc_extra_ion_sf_info_t info = {0};
            gralloc_extra_query(handle, GRALLOC_EXTRA_GET_IOCTL_ION_SF_INFO, &info);
            gralloc_extra_sf_set_status(&info, GRALLOC_EXTRA_MASK_YUV_COLORSPACE, GRALLOC_EXTRA_BIT_YUV_BT601_FULL);
            gralloc_extra_perform(handle, GRALLOC_EXTRA_SET_IOCTL_ION_SF_INFO, &info);
        }
        else
        {
            retImage = allocator->alloc_ion(TAG, imgParam);
        }

        if( NULL == retImage.get() ) {
            return false;
        }

        if(isGB) {
            if ( !retImage->lockBuf( TAG, gbusage ) )
            {
                __android_log_write(ANDROID_LOG_ERROR, TAG, "lock GBuffer failed");
                return false;
            }
        } else {
            if ( !retImage->lockBuf( TAG, eBUFFER_USAGE_SW_MASK | eBUFFER_USAGE_HW_MASK ) )
            {
                __android_log_write(ANDROID_LOG_ERROR, TAG, "lock Buffer failed");
                return false;
            }
        }

        // if ( !retImage->syncCache( eCACHECTRL_INVALID ) )
        // {
        //     __android_log_write(ANDROID_LOG_ERROR, TAG, "syncCache failed");
        //     return false;
        // }

        return true;
    }

    static bool freeImageBuffer(const char *TAG, sp<IImageBuffer>& imgBuf)
    {
        if( NULL == TAG
            || 0 == strlen(TAG)
            || NULL == imgBuf.get() )
        {
            return false;
        }

        IImageBufferAllocator* allocator = IImageBufferAllocator::getInstance();
        if( !imgBuf->unlockBuf( TAG ) )
        {
            __android_log_write(ANDROID_LOG_ERROR, TAG, "unlock Buffer failed");
            return false;
        }

        allocator->free(imgBuf.get());
        imgBuf = NULL;

        return true;
    }

    static bool rotateBuffer(const char *TAG,
                             const MUINT8 *SRC_BUFFER,
                             const MSize SRC_SIZE,
                             MUINT8 *dstBuffer,
                             int targetRotation,
                             StereoArea targetROI=STEREO_AREA_ZERO)
    {
        if(NULL == SRC_BUFFER ||
           NULL == dstBuffer ||
           0 == SRC_SIZE.w ||
           0 == SRC_SIZE.h)
        {
            return false;
        }

        const size_t LOG_SIZE = 100;
        char logName[LOG_SIZE];
        int err = snprintf(logName, LOG_SIZE,
                           "RotateBuffer(CPU): src size %dx%d, target ROI (%d, %d) %dx%d, rotate %d",
                           SRC_SIZE.w, SRC_SIZE.h,
                           targetROI.startPt.x, targetROI.startPt.y, targetROI.size.w, targetROI.size.h,
                           targetRotation);
        if(err < 0)
        {
            //Do nothing
        }
        AutoProfileUtil profile(TAG, logName);

        const MINT32 BUFFER_LEN = SRC_SIZE.w * SRC_SIZE.h;

        //Rotate by CPU
        MINT32 writeRow = 0;
        MINT32 writeCol = 0;
        MINT32 writePos = 0;

        if(targetROI == STEREO_AREA_ZERO) {
            if(StereoHAL::isVertical(targetRotation)) {
                targetROI.size.w = SRC_SIZE.h;
                targetROI.size.h = SRC_SIZE.w;
            } else {
                targetROI.size.w = SRC_SIZE.w;
                targetROI.size.h = SRC_SIZE.h;
            }
        }

        if(0 == targetRotation) {
            if(targetROI.size.w != SRC_SIZE.w ||
               targetROI.size.h != SRC_SIZE.h)
            {
                ::memset(dstBuffer, 0, sizeof(*dstBuffer)*targetROI.size.w * targetROI.size.h);
                dstBuffer += targetROI.startPt.y * targetROI.size.w + targetROI.startPt.x;
                for(writeRow = 0; writeRow < SRC_SIZE.h; ++writeRow) {
                    ::memcpy(dstBuffer, SRC_BUFFER, SRC_SIZE.w);
                    dstBuffer += targetROI.size.w;
                    SRC_BUFFER += SRC_SIZE.w;
                }
            } else {
                ::memcpy(dstBuffer, SRC_BUFFER, BUFFER_LEN);
            }
        } else if(90 == targetRotation) {
            writeRow = SRC_SIZE.w;  //only for counting
            writeCol = (targetROI.startPt.y+SRC_SIZE.w-1) * targetROI.size.w + targetROI.startPt.x;
            writePos = writeCol;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                *(dstBuffer + writePos) = *(SRC_BUFFER + i);

                writePos -= targetROI.size.w;
                --writeRow;
                if(writeRow <= 0) {
                    writeRow = SRC_SIZE.w;
                    ++writeCol;
                    writePos = writeCol;
                }
            }
        } else if(270 == targetRotation) {
            writeRow = 0;
            writeCol = targetROI.startPt.y * targetROI.size.w + targetROI.startPt.x + SRC_SIZE.h - 1;
            writePos = writeCol;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                *(dstBuffer + writePos) = *(SRC_BUFFER + i);

                writePos += targetROI.size.w;
                ++writeRow;
                if(writeRow >= SRC_SIZE.w) {
                    writeRow = 0;
                    --writeCol;
                    writePos = writeCol;
                }
            }
        } else if(180 == targetRotation) {
            writeRow = targetROI.startPt.y;
            writeCol = 0;
            writePos = writeRow * targetROI.size.w + targetROI.startPt.x;
            for(int i = BUFFER_LEN-1; i >= 0; --i) {
                *(dstBuffer + writePos) = *(SRC_BUFFER + i);

                ++writePos;
                ++writeCol;
                if(writeCol >= SRC_SIZE.w) {
                    ++writeRow;
                    writeCol = 0;
                    writePos += targetROI.padding.w;
                }
            }
        }

        return true;
    }

    static bool rotateBuffer(const char *TAG,
                             IImageBuffer *srcImage,
                             IImageBuffer *dstImage,
                             int targetRotation,
                             StereoArea targetROI=STEREO_AREA_ZERO)
    {
        if(NULL == srcImage ||
           NULL == dstImage)
        {
            return false;
        }

        bool rotateByCPU = false;
        MSize SRC_SIZE = srcImage->getImgSize();
        MSize DST_SIZE = dstImage->getImgSize();

        // Check if resize is needed or not. If not, use CPU.
        if(targetROI.size.w != 0 ||
           targetROI.size.h != 0)
        {
            DST_SIZE = targetROI.contentSize();
        }

        if(StereoHAL::isVertical(targetRotation))
        {
            if(SRC_SIZE.w <= DST_SIZE.h &&
               SRC_SIZE.h <= DST_SIZE.w)
            {
                rotateByCPU = true;
            }
        }
        else
        {
            if(SRC_SIZE.w <= DST_SIZE.w &&
               SRC_SIZE.h <= DST_SIZE.h)
            {
                rotateByCPU = true;
            }
        }

        if(rotateByCPU)
        {
            return rotateBuffer(LOG_TAG,
                                (MUINT8*)srcImage->getBufVA(0),
                                SRC_SIZE,
                                (MUINT8*)dstImage->getBufVA(0),
                                StereoSettingProvider::getModuleRotation(),
                                targetROI);
        }

        const size_t LOG_SIZE = 100;
        char logName[LOG_SIZE];
        int err = snprintf(logName, LOG_SIZE,
                           "RotateBuffer(MDP): src size %dx%d, dst size %dx%d, target ROI (%d, %d) %dx%d rotate %d",
                           SRC_SIZE.w, SRC_SIZE.h, dstImage->getImgSize().w, dstImage->getImgSize().h,
                           targetROI.startPt.x, targetROI.startPt.y, targetROI.size.w, targetROI.size.h,
                           targetRotation);
        if(err < 0)
        {
            // Do nothing
        }
        AutoProfileUtil profile(TAG, logName);

        targetROI.rotate(targetRotation);

        if(targetROI.size.w != 0 ||
           targetROI.size.h != 0)
        {
            if(StereoHAL::isVertical(targetRotation))
            {
                targetROI.rotate((360 - targetRotation), false);
            }

            MSize contentSize = targetROI.contentSize();
            DpRect dstROI(targetROI.startPt.x, targetROI.startPt.y, contentSize.w, contentSize.h);
            return transformImage(srcImage, dstImage, targetRotation, NULL, &dstROI);
        } else {
            return transformImage(srcImage, dstImage, targetRotation);
        }
    }

    static bool removePadding(IImageBuffer *src,
                              IImageBuffer *dst,
                              StereoArea &srcArea,
                              const bool USE_MDP=false)
    {
        MSize dstSize = dst->getImgSize();
        if(dstSize.w > srcArea.size.w ||
           dstSize.h > srcArea.size.h)
        {
            return false;
        }

        MUINT8 *pImgIn  = (MUINT8*)src->getBufVA(0);
        MUINT8 *pImgOut = (MUINT8*)dst->getBufVA(0);
        ::memset(pImgOut, 0, dstSize.w * dstSize.h);
        //Copy Y
        pImgIn = pImgIn + srcArea.size.w * srcArea.startPt.y + srcArea.startPt.x;
        for(int i = dstSize.h-1; i >= 0; --i, pImgOut += dstSize.w, pImgIn += srcArea.size.w)
        {
            ::memcpy(pImgOut, pImgIn, dstSize.w);
        }

        //Copy UV
        if(src->getImgFormat() == eImgFmt_YV12 &&
           dst->getImgFormat() == eImgFmt_YV12 &&
           src->getPlaneCount() == 3 &&
           dst->getPlaneCount() == 3)
        {
            dstSize.w /= 2;
            dstSize.h /= 2;
            srcArea /= 2;
            for(int p = 1; p < 3; ++p) {
                pImgIn = (MUINT8*)src->getBufVA(p);
                pImgOut = (MUINT8*)dst->getBufVA(p);
                ::memset(pImgOut, 0, dstSize.w * dstSize.h);

                pImgIn = pImgIn + srcArea.size.w * srcArea.startPt.y + srcArea.startPt.x;
                for(int i = dstSize.h-1; i >= 0; --i, pImgOut += dstSize.w, pImgIn += srcArea.size.w)
                {
                    ::memcpy(pImgOut, pImgIn, dstSize.w);
                }
            }
        }

        return true;
    }

private:
    static const MUINT32  PLANE_COUNT = 3;
    static const bool     DO_FLUSH = true;

    static DpColorFormat __getDpImageFormat(IImageBuffer *img) {
        if(img) {
            //Only support YV12, RGBA, Y8, NV12, NV21
            switch(img->getImgFormat()) {
            case eImgFmt_YV12:
                return DP_COLOR_YV12;
                break;
            case eImgFmt_RGBA8888:
                return DP_COLOR_RGBA8888;
                break;
            case eImgFmt_Y8:
                return DP_COLOR_GREY;
                break;
            case eImgFmt_NV12:
                return DP_COLOR_NV12;
                break;
            case eImgFmt_NV21:
                return DP_COLOR_NV21;
                break;
            case eImgFmt_YUY2:
                return DP_COLOR_YUY2;
            case eImgFmt_STA_2BYTE:
                // just need the 2 bytes per pixel, so use this maping: eImgFmt_STA_2BYTE -> DP_COLOR_RGB565_RAW
                return DP_COLOR_RGB565_RAW;
                break;
            case eImgFmt_I420:
                return DP_COLOR_I420;
                break;
            default:
                break;
            }
        }

        return DP_COLOR_UNKNOWN;
    }

    static bool __setSrc(DpIspStream &dpStream, IImageBuffer *imgSrc, DpRect *srcROI)
    {
        if(NULL == imgSrc) {
            return false;
        }

        DpColorFormat imgFormat = __getDpImageFormat(imgSrc);
        if(DP_COLOR_UNKNOWN == imgFormat) {
            return false;
        }

        // Queue buffer
        MUINT32      srcPAs[3]  = {0, 0, 0};
        unsigned int bytes[3]   = {0, 0, 0};
        size_t       planeCount = imgSrc->getPlaneCount();

        for(size_t p = 0; p < planeCount; ++p)
        {
            srcPAs[p] = imgSrc->getBufPA(p);
            bytes[p]  = imgSrc->getBufSizeInBytes(p);
        }

        auto status = dpStream.queueSrcBuffer(srcPAs, bytes, planeCount);
        CHECK_DP_SUCCESS(queueSrcBuffer, status);

        // Set config
        dpStream.setSrcConfig(imgSrc->getImgSize().w,
                              imgSrc->getImgSize().h,
                              imgSrc->getBufStridesInBytes(0),
                              (imgSrc->getPlaneCount() <= 1) ? 0 : imgSrc->getBufStridesInBytes(1),
                              imgFormat,
                              DP_PROFILE_FULL_BT601,
                              eInterlace_None,
                              srcROI,
                              DO_FLUSH,
                              DP_SECURE_NONE);
        CHECK_DP_SUCCESS(setSrcConfig, status);

        return true;
    }

    static bool __setDst(DpIspStream &dpStream,
                         IImageBuffer *imgDst,
                         int eRotateDegree = 0,
                         DpRect *dstROI = NULL,
                         const int PORT_INDEX = 0)
    {
        if(NULL == imgDst) {
            return false;
        }

        // Set dst
        DpColorFormat imgFormat = __getDpImageFormat(imgDst);
        if(DP_COLOR_UNKNOWN == imgFormat) {
            return false;
        }

        // Queue buffer
        MUINT32      dstPAs[3]  = {0, 0, 0};
        unsigned int bytes[3]   = {0, 0, 0};
        size_t       planeCount = imgDst->getPlaneCount();

        for(size_t p = 0; p < planeCount; ++p)
        {
            dstPAs[p] = imgDst->getBufPA(p);
            bytes[p]  = imgDst->getBufSizeInBytes(p);
        }

        auto status = dpStream.queueDstBuffer(PORT_INDEX, dstPAs, bytes, planeCount);
        CHECK_DP_SUCCESS(queueDstBuffer, status);

        // Set config
        if(dstROI) {
            status = dpStream.setDstConfig(PORT_INDEX,
                                           dstROI->w,
                                           dstROI->h,
                                           imgDst->getBufStridesInBytes(0),
                                           (imgDst->getPlaneCount() <= 1) ? 0 : imgDst->getBufStridesInBytes(1),
                                           imgFormat,
                                           DP_PROFILE_FULL_BT601,
                                           eInterlace_None,
                                           dstROI,
                                           DO_FLUSH,
                                           DP_SECURE_NONE);
        } else {
            status = dpStream.setDstConfig(PORT_INDEX,
                                           imgDst->getImgSize().w,
                                           imgDst->getImgSize().h,
                                           imgDst->getBufStridesInBytes(0),
                                           (imgDst->getPlaneCount() <= 1) ? 0 : imgDst->getBufStridesInBytes(1),
                                           imgFormat,
                                           DP_PROFILE_FULL_BT601,
                                           eInterlace_None,
                                           NULL,
                                           DO_FLUSH,
                                           DP_SECURE_NONE);
        }
        CHECK_DP_SUCCESS(setDstConfig, status);

        status = dpStream.setRotation(PORT_INDEX, eRotateDegree);
        CHECK_DP_SUCCESS(setRotate, status);

        return true;
    }
};
#endif
