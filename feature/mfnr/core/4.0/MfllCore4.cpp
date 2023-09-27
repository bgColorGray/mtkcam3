/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#define LOG_TAG "MfllCore4"

#include <mtkcam3/feature/mfnr/MfllLog.h>
#include <mtkcam/aaa/IHalISP.h>


#include "MfllCore4.h"

// MFNR Core
#include <MfllOperationSync.h>
#include <MfllUtilities.h>


// AOSP
#include <cutils/compiler.h>

// STL
#include <chrono>

using namespace mfll;

constexpr int32_t ver_major      = 4;
constexpr int32_t ver_minor      = 0;
constexpr int32_t ver_featured   = static_cast<int32_t>(IMfllCore::Type::DEFAULT);


MfllCore4::MfllCore4() : MfllCore()
{
    m_aryMemcFrameLevelConf.clear();
    m_aryMemcFrameLevelConf.resize(MFLL_MAX_FRAMES, 0);
}


enum MfllErr MfllCore4::releaseBuffer(const enum MfllBuffer& s, int index /* = 0 */)
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
    else if (s == MfllBuffer_DifferenceImage) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return MfllErr_BadArgument;
        }
        m_imgDifferenceImage[index].releaseBufLocked();
    }
    else if (s == MfllBuffer_MationCompensationMv) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return MfllErr_BadArgument;
        }
        m_imgMotionCompensationMvs[index].releaseBufLocked();
    }
    else if (s == MfllBuffer_DCESOWorking) {
        m_imgDcesoBuf[0].releaseBufLocked();
    }
    else if (s == MfllBuffer_MixedYuvDebug) {
        m_imgMixDebug.releaseBufLocked();
    }
    else {
        err = MfllCore::releaseBuffer(s, index);
    }

    return err;
}


sp<IMfllImageBuffer> MfllCore4::retrieveBuffer(const enum MfllBuffer& s, int index /* = 0 */)
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
    else if (s == MfllBuffer_DifferenceImage) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return rImg;
        }
        rImg = m_imgDifferenceImage[index].getImgBufLocked();
    }
    else if (s == MfllBuffer_MationCompensationMv) {
        if (CC_UNLIKELY(index >= static_cast<int>(getBlendFrameNum()) || index < 0)) {
            mfllLogE("index:%d out of range (max:%u)", index, getBlendFrameNum());
            return rImg;
        }
        rImg = m_imgMotionCompensationMvs[index].getImgBufLocked();
    }
    else if (s == MfllBuffer_DCESOWorking) {
        rImg = m_imgDcesoBuf[0].getImgBufLocked();
    }
    else if (s == MfllBuffer_MixedYuvDebug) {
        rImg = m_imgMixDebug.getImgBufLocked();
    }
    else {
        rImg = MfllCore::retrieveBuffer(s, index);
    }

    return rImg;
}


enum MfllErr MfllCore4::do_Init(const MfllConfig_t& cfg)
{
    mfllLogD("MfllCore4: %s", __FUNCTION__);
    return MfllCore::do_Init(cfg);
}

enum MfllErr MfllCore4::do_AllocMssBuffer(void* void_index)
{
    const unsigned int index = (unsigned int)(long)(void_index);
    enum MfllErr err = MfllErr_Ok;

    err = MfllCore::do_AllocMssBuffer(void_index);

    size_t pSize = m_imgMssYuvs[index].getSize();
    if (CC_UNLIKELY(pSize == 0)) {
        err = MfllErr_UnexpectedError;
    }

    // allocate confidence maps
    if (CC_LIKELY(err == MfllErr_Ok)) {
        //
        // lock as critical section
        std::lock_guard<std::mutex> __l(m_imgDifferenceImage[index].locker);

        sp<IMfllImageBuffer> pImg = m_imgDifferenceImage[index].imgbuf;

        // if buffer hasn't been initialized, init it
        if (pImg.get() == nullptr) {
            mfllLogD3("allocate confidence map %u", index);
            // create confidence map
            pImg = createDifferenceImage(
                (size_t)m_imgMssYuvs[index].images[pSize-1].imgbuf->getWidth(),
                (size_t)m_imgMssYuvs[index].images[pSize-1].imgbuf->getHeight());
            if (CC_UNLIKELY(pImg.get() == nullptr)) {
                mfllLogE("create IMfllImageBuffer failed");
                err = MfllErr_UnexpectedError;
            }
            else {
                m_imgDifferenceImage[index].imgbuf = pImg;
            }
        }
        else {
            mfllLogD3("difference image has been initialized, ignore");
        }
    }

lbExit:
    return err;
}

enum MfllErr MfllCore4::do_AllocMemcWorking(void* void_index)
{
    const unsigned int index = (unsigned int)(long)(void_index);
    enum MfllErr err = MfllErr_Ok;

    err = MfllCore::do_AllocMemcWorking(void_index);

    // allocate confidence maps
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

    // allocate motion compensation mv
    if (CC_LIKELY(err == MfllErr_Ok)) {
        //
        // lock as critical section
        std::lock_guard<std::mutex> __l(m_imgMotionCompensationMvs[index].locker);

        sp<IMfllImageBuffer> pImg = m_imgMotionCompensationMvs[index].imgbuf;

        // if buffer hasn't been initialized, init it
        if (pImg.get() == nullptr) {
            mfllLogD3("allocate MotionCompensationMv %u", index);
            // create confidence map
            pImg = createMotionCompensationMV((size_t)m_width, (size_t)m_height);
            if (CC_UNLIKELY(pImg.get() == nullptr)) {
                mfllLogE("create IMfllImageBuffer failed");
                err = MfllErr_UnexpectedError;
            }
            else {
                m_imgMotionCompensationMvs[index].imgbuf = pImg;
            }
        }
        else {
            mfllLogD3("MC MV has been initialized, ignore");
        }
    }

lbExit:
    return err;
}

enum MfllErr MfllCore4::do_AllocYuvGolden(JOB_VOID)
{
    enum MfllErr err = MfllErr_Ok;

    err = MfllCore::do_AllocYuvGolden(__arg);
#if 0
//Using DECSO from P2A
    // allocate DCESO buffer
    if (CC_LIKELY(err == MfllErr_Ok)) {
        //
        // lock as critical section
        std::lock_guard<std::mutex> __l(m_imgDCESO.locker);

        sp<IMfllImageBuffer> pImg = m_imgDCESO.imgbuf;

        // if buffer hasn't been initialized, init it
        if (pImg.get() == nullptr) {
            mfllLogD3("allocate DCESO buffer");
            // create confidence map
            pImg = createDcesoBuffer();
            if (CC_UNLIKELY(pImg.get() == nullptr)) {
                mfllLogE("create IMfllImageBuffer failed");
                err = MfllErr_UnexpectedError;
            }
            else {
                m_imgDCESO.imgbuf = pImg;
            }
        }
        else {
            mfllLogD3("DCESO buffer been initialized, ignore");
        }
    }
#endif
    return err;
}

enum MfllErr MfllCore4::do_MotionEstimation(void* void_index)
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
        // MEMC v1.3 new APIs
        memc->setMotionCompensationMvImage(m_imgMotionCompensationMvs[index].imgbuf);
        memc->setCurrentIso(m_iso);
        //for dump
        MfllMiddlewareInfo_t __middlewareInfo = getMiddlewareInfo();
        memc->setMemcDump(__middlewareInfo.uniqueKey, 0, int32_t(index));

        err = memc->motionEstimation();
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            memc->giveupMotionCompensation();
            mfllLogE("%s: IMfllMemc::motionEstimation failed, returns %d", __FUNCTION__, (int)err);
            goto lbExit;
        }

        m_aryMemcFrameLevelConf[index] = memc->getMeFrameLevelConfidence();
        mfllLogD3("Update m_aryMemcFrameLevelConf[%u] = %d", index, m_aryMemcFrameLevelConf[index]);
    }

lbExit:
    return err;
}


enum MfllErr MfllCore4::do_MotionCompensation(void* void_index)
{
#if 1
    return MfllErr_Ok;
#else
    unsigned int index = (unsigned int)(long)void_index;
    auto err = MfllCore::do_MotionCompensation(void_index);
    // if compensation is OK and confidence map exists, we need sync it from
    // CPU cache buffer chunk to physical buffer chunk
    if (CC_LIKELY(err == MfllErr_Ok && m_imgConfidenceMaps[index].imgbuf.get())) {
        err = m_imgConfidenceMaps[index].imgbuf->syncCache();
    }
    return err;
#endif
}

enum MfllErr MfllCore4::do_EncodeMss(void *void_index)
{
    enum MfllErr err = MfllErr_Ok;

    const unsigned int index = (unsigned int)(long)void_index;

    MfllOperationSync::getInstance()->addJob(MfllOperationSync::JOB_MSS);

    if (CC_UNLIKELY(index >= MFLL_MAX_FRAMES)){
        mfllLogE("index:%d is out of frames index:%d, ignore it", index, MFLL_MAX_FRAMES);
        return MfllErr_BadArgument;
    }
    else {
        /* capture YUV */
        /* no raw or ignored */
        err = doAllocMssBuffer((void*)(long)index);

        // check if buffers are ready to use && qyuvs[index] exists
        bool bBuffersReady = [&](){
            if (err != MfllErr_Ok)
                return false;
            size_t start = (index)?0:2;
            for (size_t i = start ; i < m_imgMssYuvs[index].getSize() ; i++)
                if (m_imgMssYuvs[index].images[i].imgbuf.get() == nullptr)
                    return false;

            /// otherwise, we need it
            return true;
        }();

        if (CC_UNLIKELY( ! bBuffersReady )) {
            mfllLogE("%s: alloc MSS buffer %u failed", __FUNCTION__, index);
            goto lbExit;
        }
        else {
            if (CC_UNLIKELY(m_spMfb.get() == NULL)) {
                mfllLogD3("%s: create IMfllMfb instance", __FUNCTION__);
                m_spMfb = IMfllMfb::createInstance();
                if (m_spMfb.get() == NULL) {
                    mfllLogE("%s: m_spMfb is NULL", __FUNCTION__);
                    err = MfllErr_CreateInstanceFailed;
                    goto lbExit;
                }
            }

            /* tell MfllMfb what shot mode and post NR type is using */
            err = m_spMfb->init(m_sensorId);
            m_spMfb->setShotMode(m_shotMode);
            m_spMfb->setPostNrType(m_postNrType);

            // assign full yuv to s0 for golden frame
            if (index == 0)
                m_imgMssYuvs[index].images[0].imgbuf = m_imgYuvs[index].imgbuf;

            sp<IMfllImageBuffer> mcmv = (index == 0)?nullptr:m_imgMotionCompensationMvs[index-1].getImgBufLocked();
            vector<IMfllImageBuffer *> pyramid = m_imgMssYuvs[index].getBuffers();
            sp<IMfllImageBuffer> omc = (index == 0)?nullptr:m_imgYuvs[index].getImgBufLocked();
            sp<IMfllImageBuffer> dceso = m_imgDcesoBuf[0].getImgBufLocked();

            err = m_spMfb->mss(
                pyramid,
                omc.get(),
                mcmv.get(),
                dceso.get(),
                index,
                YuvStage_Mss
                );

            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: Encode Mss Pyramid fail", __FUNCTION__);
                goto lbExit;
            }
        }
    }

lbExit:
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogD("%s: bypass MEMC/BLD (index = %u)", __FUNCTION__, index);
        m_bypass.bypassMotionEstimation[index] = 1;
        m_bypass.bypassMotionCompensation[index] = 1;
        m_bypass.bypassBlending[index] = 1;
    }

    return err;
}

enum MfllErr MfllCore4::do_Msf(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;

   {
        MfllOperationSync::getInstance()->addJob(MfllOperationSync::JOB_MSF);

        err = doAllocYuvWorking(NULL);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate YUV working buffer failed", __FUNCTION__);
            goto lbExit;
        }

        err = doAllocWeighting((void*)(long)0);
        err = doAllocWeighting((void*)(long)1);
        err = doAllocWeighting((void*)(long)2);
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
        mfllLogD3("%s: do Msf now", __FUNCTION__);

//to-do: ping-pong buffer
        vector<IMfllImageBuffer *> base     = m_ptrImgYuvBase->getBuffers();
        vector<IMfllImageBuffer *> ref      = m_ptrImgYuvRef->getBuffers();
        vector<IMfllImageBuffer *> out      = m_ptrImgYuvBlended->getBuffers();
        vector<IMfllImageBuffer *> wt_in    = m_ptrImgWeightingIn->getBuffers();
        vector<IMfllImageBuffer *> wt_out   = m_ptrImgWeightingOut->getBuffers();
        vector<IMfllImageBuffer *> wt_ds    = m_ptrImgWeightingDs->getBuffers();
        sp<IMfllImageBuffer>       dceso    = m_imgDcesoBuf[0].getImgBufLocked();


        sp<IMfllImageBuffer> difference = m_imgDifferenceImage[index].getImgBufLocked();
        sp<IMfllImageBuffer> confmap = m_imgConfidenceMaps[index].getImgBufLocked();
        IMfllImageBuffer* dsFram = base.back() ? base.back() : NULL;

        // allocate debug buffer if needed
        if (m_ptrImgYuvBlended->getSize() > 0) {
            sp<IMfllImageBuffer> out_ref = m_ptrImgYuvBlended->images[0].getImgBufLocked();
            if (out_ref.get() != nullptr) {
                if (CC_UNLIKELY( MfllProperty::readProperty(Property_DumpYuv) > 0 )) {
                    mfllLogD("%s: create msf-p2 debug buffer for dump", __FUNCTION__);

                    // lock as critical section
                    std::lock_guard<std::mutex> __l(m_imgMixDebug.locker);
                    sp<IMfllImageBuffer> pImg = m_imgMixDebug.imgbuf;

                    mfllLogD3("allocate msf-p2 debug buffer");
                    // create confidence map
                    pImg = createMixDebugBuffer(out_ref);
                    if (CC_UNLIKELY(pImg.get() == nullptr)) {
                        mfllLogE("create IMfllImageBuffer failed");
                    }
                    m_imgMixDebug.imgbuf = pImg;
                }
                else {
                    m_imgMixDebug.releaseBufLocked();
                }
            }
        }

        //debug dump for msf-p2
        sp<IMfllImageBuffer> debug = m_imgMixDebug.getImgBufLocked();
        m_spMfb->setMixDebug(debug);

#if 1
        mfllLogD3("%s: syncCache", __FUNCTION__);
        // CPU -> HW
        if (difference.get())
            difference->syncCache();
        if (confmap.get())
            confmap->syncCache();
        if (dsFram)
            dsFram->syncCache();
#endif

        /**
         * while index == 0, which means the first time to blend, the input weighting
         * table should be sent
         */
        err = m_spMfb->msf(
            base,
            ref,
            out,
            wt_in,
            wt_out,
            wt_ds,
            difference.get(),
            confmap.get(),
            dsFram,
            dceso.get(),
            index,
            YuvStage_Msf
        );

        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: Mfb failed with code %d", __FUNCTION__, (int)err);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
        else {
            m_blendedCount++; // count blended frame
        }
    }

lbExit:
    return err;

}

unsigned int MfllCore4::getVersion()
{
    return MFLL_MAKE_REVISION(ver_major, ver_minor, ver_featured);
}


std::string MfllCore4::getVersionString()
{
    return mfll::makeRevisionString(ver_major, ver_minor, ver_featured);
}


vector<int> MfllCore4::getMemcFrameLvConfs()
{
    return m_aryMemcFrameLevelConf;
}

android::sp<IMfllImageBuffer>
MfllCore4::createDifferenceImage(
        size_t width,
        size_t height
        )
{
    mfllLogD3("%s: difference image  size = %zux%zu", __FUNCTION__, width, height);
    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();

    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create difference image  IMfllImageBuffer failed");
        return nullptr;
    }
    m->setImageFormat(ImageFormat_Nv12);
    m->setResolution(width, height);
    m->setAligned(16, 16); // 1 pixel align, hardware support odd pixel
    auto err = m->initBuffer();
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogE("create difference image failed");
        return nullptr;
    }
    return m;
}

android::sp<IMfllImageBuffer>
MfllCore4::createConfidenceMap(
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
    // block based, where the size of block is 16x16 (default)
    // then do 2-pixel alignment for msf's requirement
    width = __ALIGN((width/blockSize), 2); //width /= blockSize;
    height = __ALIGN((height/blockSize), 2); //height /= blockSize;
#undef __ALIGN

    mfllLogD3("%s: Confidence Map size = %zux%zu", __FUNCTION__, width, height);
    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();

    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create confidence map IMfllImageBuffer failed");
        return nullptr;
    }
    m->setImageFormat(ImageFormat_Sta8);
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
MfllCore4::createMotionCompensationMV(
        size_t width,
        size_t height
        )
{
    /* Size of CompensationMV                        *
     * width  = (((full_width ) + 15) >> 4 << 4) *
     * height = (((full_height ) + 15) >> 4 << 4) */
    static const size_t blockSize = 16;
#define __ALIGN(w, a) (((w + (a-1)) / a) * a)
    // 16 pixel algined first
    width = __ALIGN(width , blockSize);
    height = __ALIGN(height, blockSize);
#undef __ALIGN

    // block based, where the size of block is 16x16 (default)
    width /= blockSize;
    height /= blockSize;

    mfllLogD3("%s: MotionCompensationMV size = %zux%zux4", __FUNCTION__, width, height);
    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();

    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create MotionCompensationMV IMfllImageBuffer failed");
        return nullptr;
    }
    m->setImageFormat(ImageFormat_Sta32);
    m->setResolution(width, height);
    m->setAligned(4, 1);
    auto err = m->initBuffer();
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogE("create MotionCompensationMV failed");
        return nullptr;
    }
    return m;
}

android::sp<IMfllImageBuffer>
MfllCore4::createDcesoBuffer()
{

    NS3Av3::Buffer_Info info;
    {
        /* RAII for IHalISP instance */
        std::unique_ptr< NS3Av3::IHalISP, std::function<void(NS3Av3::IHalISP*)> > pHalISP;

        pHalISP = decltype(pHalISP)(
            MAKE_HalISP(m_sensorId, LOG_TAG),
            [](NS3Av3::IHalISP* p){ if (p) p->destroyInstance(LOG_TAG); }
        );

        if (CC_UNLIKELY( pHalISP.get() == nullptr )) {
            mfllLogE("create IHalISP fail");
            return nullptr;
        }

        MBOOL ret = pHalISP->queryISPBufferInfo(info);
        pHalISP = nullptr;
    }

    if (CC_UNLIKELY( !info.DCESO_Param.bSupport )) {
        mfllLogE("queryISPBufferInfo fail or DCESO not support($d)", info.DCESO_Param.bSupport);
        return nullptr;
    }

    sp<IMfllImageBuffer> m = IMfllImageBuffer::createInstance();
    if (CC_UNLIKELY(m.get() == nullptr)) {
        mfllLogE("create DCESO IMfllImageBuffer failed");
        return nullptr;
    }

    size_t width = info.DCESO_Param.size.w;
    size_t height = info.DCESO_Param.size.h;
    ImageFormat format = m->tranImageFormat(info.DCESO_Param.format);

    mfllLogD("%s: DCESO size = %zux%zu, fmt = %d, mfll_mft = %d", __FUNCTION__, width, height, info.DCESO_Param.format, format);

    if (CC_UNLIKELY( format == ImageFormat_Unknown )) {
        mfllLogE("DCESO format is not support($d)", info.DCESO_Param.format);
        return nullptr;
    }

    m->setImageFormat(format);
    m->setResolution(width, height);
    m->setAligned(1, 1); // 1 pixel align, hardware support odd pixel
    auto err = m->initBuffer();
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogE("create DCESO failed");
        return nullptr;
    }
    return m;
}

android::sp<IMfllImageBuffer>
MfllCore4::createMixDebugBuffer(android::sp<IMfllImageBuffer> ref)
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
