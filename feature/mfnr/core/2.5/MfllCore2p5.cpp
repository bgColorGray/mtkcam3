/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2017. All rights reserved.
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
#define LOG_TAG "MfllCore2p5"

#include <mtkcam3/feature/mfnr/MfllLog.h>

#include "MfllCore2p5.h"

// MFNR Core
#include <MfllOperationSync.h>
#include <MfllUtilities.h>


// AOSP
#include <cutils/compiler.h>

// STL
#include <chrono>

using namespace mfll;

constexpr int32_t ver_major      = 2;
constexpr int32_t ver_minor      = 5;
constexpr int32_t ver_featured   = static_cast<int32_t>(IMfllCore::Type::DEFAULT);

MfllCore2p5::MfllCore2p5() : MfllCore2()
{
}


enum MfllErr MfllCore2p5::releaseBuffer(const enum MfllBuffer& s, int index /* = 0 */)
{
    MfllErr err = MfllErr_Ok;
    // check if MfllBuffer_ConfidenceMap first.
    if (s == MfllBuffer_ConfidenceMap) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return MfllErr_BadArgument;
        }
        m_imgConfidenceMaps[index].releaseBufLocked();
    }
    else if (s == MfllBuffer_MixedYuvDebug) {
        m_imgMixDebug.releaseBufLocked();
    }
    else {
        err = MfllCore::releaseBuffer(s, index);
    }

    return err;
}


sp<IMfllImageBuffer> MfllCore2p5::retrieveBuffer(const enum MfllBuffer& s, int index /* = 0 */)
{
    MfllErr err = MfllErr_Ok;
    sp<IMfllImageBuffer> rImg;

    // check if MfllBuffer_ConfidenceMap first.
    if (s == MfllBuffer_ConfidenceMap) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return rImg;
        }
        rImg = m_imgConfidenceMaps[index].getImgBufLocked();
    }
    else if (s == MfllBuffer_MixedYuvDebug) {
        rImg = m_imgMixDebug.getImgBufLocked();
    }
    else {
        rImg = MfllCore::retrieveBuffer(s, index);
    }

    return rImg;
}


enum MfllErr MfllCore2p5::do_Init(const MfllConfig_t& cfg)
{
    mfllLogD("MfllCore2p5: %s", __FUNCTION__);
    return MfllCore::do_Init(cfg);
}


enum MfllErr MfllCore2p5::do_AllocMemcWorking(void* void_index)
{
    const unsigned int index = (unsigned int)(long)(void_index);
    enum MfllErr err = MfllErr_Ok;

    err = MfllCore::do_AllocMemcWorking(void_index);

    if (CC_LIKELY(err == MfllErr_Ok)) {
        //
        // lock as critical section
        std::lock_guard<std::mutex> __l(m_imgConfidenceMaps[index].locker);

        sp<IMfllImageBuffer> pImg = m_imgConfidenceMaps[index].imgbuf;

        // if buffer hasn't been initialized, init it
        if (pImg.get() == nullptr) {
            mfllLogD3("allocate confidence map %u", index);
            // create confidence map
            pImg = createConfidenceMap((size_t)m_width, (size_t)m_height);
            if (CC_UNLIKELY(pImg.get() == nullptr)) {
                mfllLogE("create IMfllImageBuffer failed");
                err = MfllErr_UnexpectedError;
            }
            else {
                m_imgConfidenceMaps[index].imgbuf = pImg;
            }
        }
        else {
            mfllLogD3("confidence map has been initialized, ignore");
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore2p5::do_MotionEstimation(void* void_index)
{
    unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;
    std::string _log = std::string("start ME") + std::to_string(index);

    {
        mfllAutoLog(_log.c_str());
        unsigned int memcIndex = index % getMemcInstanceNum();
        sp<IMfllMemc> memc = m_spMemc[memcIndex];
        if (CC_UNLIKELY(memc.get() == NULL)) {
            mfllLogE("%s: MfllMemc is necessary to be created first (index=%d)", __FUNCTION__, index);
            err = MfllErr_NullPointer;
            goto lbExit;
        }

        err = doAllocMemcWorking((void*)(long)memcIndex);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate MEMC working buffer(%d) failed", __FUNCTION__, memcIndex);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        /* set motion vector */
        memc->setMotionVector(m_globalMv[index + 1].x, m_globalMv[index + 1].y);
        memc->setAlgorithmWorkingBuffer(m_imgMemc[memcIndex].imgbuf);
        memc->setMeBaseImage(m_imgQYuvs[0].imgbuf);
        memc->setMeRefImage(m_imgQYuvs[index + 1].imgbuf);
        // MEMC v1.1 new APIs
        memc->setConfidenceMapImage(m_imgConfidenceMaps[index].imgbuf);
        memc->setCurrentIso(m_iso);

        err = memc->motionEstimation();
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            memc->giveupMotionCompensation();
            mfllLogE("%s: IMfllMemc::motionEstimation failed, returns %d", __FUNCTION__, (int)err);
            goto lbExit;
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore2p5::do_MotionCompensation(void* void_index)
{
    unsigned int index = (unsigned int)(long)void_index;
    auto err = MfllCore::do_MotionCompensation(void_index);
    // if compensation is OK and confidence map exists, we need sync it from
    // CPU cache buffer chunk to physical buffer chunk
    if (CC_LIKELY(err == MfllErr_Ok && m_imgConfidenceMaps[index].imgbuf.get())) {
        err = m_imgConfidenceMaps[index].imgbuf->syncCache();
    }
    return err;
}


enum MfllErr MfllCore2p5::do_Blending(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;

   {
        MfllOperationSync::getInstance()->addJob(MfllOperationSync::JOB_MFB);

        err = doAllocYuvBase(NULL);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate YUV base buffer failed", __FUNCTION__);
            goto lbExit;
        }
        err = doAllocYuvWorking(NULL);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate YUV working buffer failed", __FUNCTION__);
            goto lbExit;
        }

        err = doAllocWeighting((void*)(long)0);
        err = doAllocWeighting((void*)(long)1);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate weighting buffer 0 or 1 failed", __FUNCTION__);
            goto lbExit;
        }

        if (CC_UNLIKELY(m_spMfb.get() == NULL)) {
            mfllLogD3("%s: create IMfllMfb instance", __FUNCTION__);
            m_spMfb = IMfllMfb::createInstance();
            if (m_spMfb.get() == NULL) {
                mfllLogE("%s: m_spMfb is NULL", __FUNCTION__);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
        }

        err = m_spMfb->init(m_sensorId);
        m_spMfb->setShotMode(m_shotMode);
        m_spMfb->setPostNrType(m_postNrType);

        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: m_spMfb init failed with code %d", __FUNCTION__, (int)err);
            goto lbExit;
        }

        /* do blending */
        mfllLogD3("%s: do blending now", __FUNCTION__);

        sp<IMfllImageBuffer> confmap = m_imgConfidenceMaps[index].getImgBufLocked();

#if 0
        mfllLogD3("%s: syncCache", __FUNCTION__);
        // CPU -> HW
        m_ptrImgYuvBase->imgbuf->syncCache();
        m_ptrImgYuvRef->imgbuf->syncCache();
        confmap->syncCache();
        if (index == 0) {
            m_ptrImgWeightingIn->imgbuf->syncCache();
        }
#endif

        /**
         * while index == 0, which means the first time to blend, the input weighting
         * table should be sent
         */
        err = m_spMfb->blend(
            m_ptrImgYuvBase->imgbuf.get(),
            m_ptrImgYuvRef->imgbuf.get(),
            confmap.get(),
            m_ptrImgYuvBlended->imgbuf.get(),
            (index == 0) ? NULL : m_ptrImgWeightingIn->imgbuf.get(),
            m_ptrImgWeightingOut->imgbuf.get()
        );

        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: Mfb failed with code %d", __FUNCTION__, (int)err);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
        else {
            /* save for mixing */
            m_ptrImgWeightingFinal = m_ptrImgWeightingOut;
            m_blendedCount++; // count blended frame
        }
    }

lbExit:
    return err;
}

enum MfllErr MfllCore2p5::do_Mixing(JOB_VOID)
{
    enum MfllErr err = MfllErr_Ok;

    {
        if (CC_UNLIKELY(m_spMfb.get() == NULL)) {
            m_spMfb = IMfllMfb::createInstance();
            if (m_spMfb.get() == NULL) {
                mfllLogE("%s: create MFB instance failed", __FUNCTION__);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
        }

        /* set sensor ID and private data for sync */
        err = m_spMfb->init(m_sensorId);
        m_spMfb->setShotMode(m_shotMode);
        m_spMfb->setPostNrType(m_postNrType);

        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: init MFB instance failed with code %d", __FUNCTION__, (int)err);
            goto lbExit;
        }

        if (m_ptrImgYuvMixed->imgbuf.get() && m_ptrImgYuvGolden->imgbuf.get()) {
            if (CC_UNLIKELY( MfllProperty::readProperty(Property_DumpYuv) > 0 )
                //MixDebug using IMG3O port, it must make sure the input and output size is the same
                && m_ptrImgYuvMixed->imgbuf->getWidth() == m_ptrImgYuvGolden->imgbuf->getWidth()) {

                mfllLogD("%s: create mix debug buffer for dump", __FUNCTION__);

                // lock as critical section
                std::lock_guard<std::mutex> __l(m_imgMixDebug.locker);
                sp<IMfllImageBuffer> pImg = m_imgMixDebug.imgbuf;

                mfllLogD3("allocate mix debug buffer");
                // create confidence map
                pImg = createMixDebugBuffer(m_ptrImgYuvMixed->getImgBufLocked());
                if (CC_UNLIKELY(pImg.get() == nullptr)) {
                    mfllLogE("create IMfllImageBuffer failed");
                }
                m_imgMixDebug.imgbuf = pImg;
            }
            else {
                m_imgMixDebug.releaseBufLocked();
            }
        }

        sp<IMfllImageBuffer> debug = m_imgMixDebug.getImgBufLocked();
        m_spMfb->setMixDebug(debug);

        /* mixing */
        mfllLogD3("%s: do mixing", __FUNCTION__);
        if ( CC_LIKELY(m_ptrImgYuvThumbnail != nullptr) &&
             CC_LIKELY(m_ptrImgYuvThumbnail->imgbuf.get() != nullptr) ){

            if (m_bDoDownscale && m_downscaleRatio > 1) {
                mfllLogD("%s: enable downscale, crop region before downscale(x:%d, y:%d, w:%d, h:%d)",
                        __FUNCTION__, m_thumbnailCropRgn.x, m_thumbnailCropRgn.y, m_thumbnailCropRgn.w, m_thumbnailCropRgn.h);

                m_thumbnailCropRgn.x = m_thumbnailCropRgn.x/m_downscaleRatio;
                m_thumbnailCropRgn.y = m_thumbnailCropRgn.y/m_downscaleRatio;
                m_thumbnailCropRgn.w = m_thumbnailCropRgn.w/m_downscaleRatio;
                m_thumbnailCropRgn.h = m_thumbnailCropRgn.h/m_downscaleRatio;
            }
            mfllLogD("%s: thumbnail crop region(x:%d, y:%d, w:%d, h:%d)",
                    __FUNCTION__, m_thumbnailCropRgn.x, m_thumbnailCropRgn.y, m_thumbnailCropRgn.w, m_thumbnailCropRgn.h);

            err = m_spMfb->mix(
                m_ptrImgYuvBlended->imgbuf.get(),
                m_ptrImgYuvGolden->imgbuf.get(),
                m_ptrImgYuvMixed->imgbuf.get(),
                m_ptrImgYuvThumbnail->imgbuf.get(),
                m_ptrImgWeightingFinal->imgbuf.get(),
                m_thumbnailCropRgn
            );
            mfllLogD("%s: thumbnail has been generated during mixing stage", __FUNCTION__);
            m_bThumbnailDone = true;
        }
        else {
            mfllLogW("%s: no thumbnail yuv", __FUNCTION__);
            err = m_spMfb->mix(
                m_ptrImgYuvBlended->imgbuf.get(),
                m_ptrImgYuvGolden->imgbuf.get(),
                m_ptrImgYuvMixed->imgbuf.get(),
                m_ptrImgWeightingFinal->imgbuf.get()
            );
        }
    }

lbExit:
    return err;
}


unsigned int MfllCore2p5::getVersion()
{
    return MFLL_MAKE_REVISION(ver_major, ver_minor, ver_featured);
}


std::string MfllCore2p5::getVersionString()
{
    return mfll::makeRevisionString(ver_major, ver_minor, ver_featured);
}


android::sp<IMfllImageBuffer>
MfllCore2p5::createConfidenceMap(
        size_t width,
        size_t height
        )
{
    /* Size of Confidence Map                        *
     * width  = (((full_width  / 2) + 15) >> 4 << 4) *
     * height = (((full_height / 2) + 15) >> 4 << 4) */
    static const size_t blockSize = 16;
#define __ALIGN(w, a) (((w + (a-1)) / a) * a)
    // 16 pixel algined first
    width = __ALIGN((width >> 1), blockSize);
    height = __ALIGN((height >> 1), blockSize);
#undef __ALIGN

    // block based, where the size of block is 16x16 (default)
    width /= blockSize;
    height /= blockSize;

    mfllLogD3("%s: Confidence Map size = %zux%zu", __FUNCTION__, width, height);
    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();

    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create confidence map IMfllImageBuffer failed");
        return nullptr;
    }
    m->setImageFormat(ImageFormat_Raw8);
    m->setResolution(width, height);
    m->setAligned(1, 1); // 1 pixel align, hardware support odd pixel
    auto err = m->initBuffer();
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogE("create confidence map failed");
        return nullptr;
    }
    return m;
}

android::sp<IMfllImageBuffer>
MfllCore2p5::createMixDebugBuffer(android::sp<IMfllImageBuffer> ref)
{
    if (CC_UNLIKELY( ref == nullptr) ) {
        mfllLogE("create Mix debug IMfllImageBuffer failed, due to ref is null");
        return nullptr;
    }

    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();
    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create Mix debug IMfllImageBuffer failed");
        return nullptr;
    }

    size_t width            = ref->getWidth();
    size_t height           = ref->getHeight();
    size_t widthAligned     = ref->getAlignedWidth();
    size_t heightAligned    = ref->getAlignedHeight();
    ImageFormat format      = ref->getImageFormat();

    mfllLogD("%s: Mix debug size = %zux%zu, aligned = %zux%zu, mfll_mft = %d", __FUNCTION__
        , width, height, widthAligned, heightAligned, format);

    m->setImageFormat(format);
    m->setResolution(width, height);
    m->setAligned(widthAligned, heightAligned);
    auto err = m->initBuffer();
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogE("create Mix debug failed");
        return nullptr;
    }
    return m;
}
