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

#ifndef __MFLLCORE_IMPL_INC__
#define __MFLLCORE_IMPL_INC__
namespace mfll {


#if !defined(MFLL_PYRAMID_SIZE) || MFLL_PYRAMID_SIZE < 2
#error "MFLL_PYRAMID_SIZE is not defined or less then 2."
#elif (MFLL_PYRAMID_SIZE > MFLL_MAX_PYRAMID_SIZE)
#error "MFLL_PYRAMID_SIZE is larger than MFLL_MAX_PYRAMID_SIZE."
#endif


enum MfllErr MfllCore::do_Init(const MfllConfig_t &cfg)
{
    MfllErr err = MfllErr_Ok;
    /* copy configuration */
    /* Update catprue frame number and blend frame number */
    m_iso = cfg.iso;
    m_exposure = cfg.exp;
    m_frameBlend = cfg.blend_num;
    m_frameCapture = cfg.capture_num;
    m_sensorId = cfg.sensor_id;
    m_shotMode = cfg.mfll_mode;
    m_rwbMode = cfg.rwb_mode;
    m_mrpMode = cfg.mrp_mode;
    m_memcMode = cfg.memc_mode;
    m_isFullSizeMc = cfg.full_size_mc != 0 ? 1 : 0;

    /**
     *  Using (number of blending - 1) as MEMC threads num
     *  or the default should be m_memcInstanceNum = MFLL_MEMC_THREADS_NUM;
     */
    m_memcInstanceNum = m_frameBlend - 1;

    /* update debug info */
    m_dbgInfoCore.frameCapture = m_frameCapture;
    m_dbgInfoCore.frameBlend = m_frameBlend;
    m_dbgInfoCore.iso = m_iso;
    m_dbgInfoCore.exp = m_exposure;
    m_dbgInfoCore.ori_iso = cfg.original_iso;
    m_dbgInfoCore.ori_exp = cfg.original_exp;
    if (isMfllMode(m_shotMode))
        m_dbgInfoCore.shot_mode = 1;
    else if (isAisMode(m_shotMode))
        m_dbgInfoCore.shot_mode = 2;
    else
        m_dbgInfoCore.shot_mode = 0;


    /* assign pointers to real buffer */
    m_ptrImgYuvBase = &(m_imgMssYuvs[0]);
    m_ptrImgYuvRef = &(m_imgMssYuvs[1]);
    m_ptrImgYuvBlended = &(m_imgMssYuvs[0]);
    m_ptrImgWeightingIn = &(m_imgWeightings[0]);
    m_ptrImgWeightingOut = &(m_imgWeightings[1]);
    m_ptrImgWeightingDs = &(m_imgWeightings[2]);

    /* init NVRAM provider */
    if (CC_UNLIKELY(m_spNvramProvider.get() == NULL)) {
        mfllLogW("%s: m_spNvramProvider has not been set, try to create one", __FUNCTION__);
        m_spNvramProvider = IMfllNvram::createInstance();
        if (CC_UNLIKELY(m_spNvramProvider.get() == NULL)) {
            err = MfllErr_UnexpectedError;
            mfllLogE("%s: create NVRAM provider failed", __FUNCTION__);
            goto lbExit;
        }
        IMfllNvram::ConfigParams nvramCfg;
        nvramCfg.iSensorId = cfg.sensor_id;
        nvramCfg.bFlashOn  = cfg.flash_on;
        m_spNvramProvider->init(nvramCfg);
    }

    /* create MEMC instance */
    for (size_t i = 0; i < getMemcInstanceNum(); i++) {
        m_spMemc[i] = IMfllMemc::createInstance();
        m_spMemc[i]->setMfllCore(this);
        // must set mfllCore before getMeDnRatio
        int32_t _MeDnRatio = m_spMemc[i]->getMeDnRatio();
        mfllLogD3("MeDnRatio:%d", _MeDnRatio);
        /* update m_qwidth and m_qheight to quater of capture resolution */
        // TODO: integrate new MEMC algo for downscale config.
        m_qwidth = m_width / _MeDnRatio;
        m_qheight = m_height / _MeDnRatio;
        mfllLogD3("(m_qwidth x m_qheight) = (%d x %d)", m_qwidth, m_qheight);

        m_spMemc[i]->setMotionEstimationResolution(m_qwidth, m_qheight);
        if (m_isFullSizeMc)
            m_spMemc[i]->setMotionCompensationResolution(m_width, m_height);
        else
            m_spMemc[i]->setMotionCompensationResolution(m_qwidth, m_qheight);

        /* set thread priority to algorithm threads */
        // {{{
        // Changes the current thread's priority, the algorithm threads will
        // inherits this value.
        int _priority = MfllProperty::readProperty(Property_AlgoThreadsPriority, MFLL_ALGO_THREADS_PRIORITY);
        int _oripriority = 0;
        // We give higher thread priority at the first MEMC thread because we
        // want the first MEMC finished ASAP.
        int _result = setThreadPriority(_priority - (i == 0 ? 1 : 0), _oripriority);
        if (CC_UNLIKELY( _result != 0 )) {
            mfllLogW("set algo threads priority failed(err=%d)", _result);
        }
        else {
            mfllLogD3("set algo threads priority to %d", _priority);
        }
        // }}}

        // init MEMC
        if (CC_UNLIKELY(m_spMemc[i]->init(m_spNvramProvider) != MfllErr_Ok)) {
            mfllLogE("%s: init MfllMemc failed with code", __FUNCTION__);
        }

        // algorithm threads have been forked,
        // if priority set OK, reset it back to the original one
        if (CC_LIKELY( _result == 0 )) {
            _result = setThreadPriority( _oripriority, _oripriority );
            if (CC_UNLIKELY( _result != 0 )) {
                mfllLogE("set priority back failed, weird!");
            }
        }

    }

    if (CC_LIKELY(m_mrpMode == MrpMode_BestPerformance)) {
        tellsFutureAllocateMemory();
    } else {
        tellsFutureAllocateMemoryLite();
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_Capture(JOB_VOID)
{
    enum MfllErr err = MfllErr_Ok;

    {
        /**
         *  N O T I C E
         *
         *  Aquire buffers should be invoked by MfllCapturer
         *  1. IMfllCore::doAllocRawBuffer
         *  2. IMfllCore::doAllocQyuvBuffer
         */
        /* check if IMfllCapturer has been assigned */
        if (CC_UNLIKELY(m_spCapturer.get() == NULL)) {
            mfllLogD3("%s: create MfllCapturer", __FUNCTION__);
            m_spCapturer = IMfllCapturer::createInstance();
            if (m_spCapturer.get() == NULL) {
                mfllLogE("%s: create MfllCapturer instance", __FUNCTION__);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
        }

        std::vector< sp<IMfllImageBuffer>* > raws;
        std::vector< sp<IMfllImageBuffer>* > rrzos;
        std::vector<MfllMotionVector_t*>     gmvs;
        std::vector< int >                   rStatus;

        /* resize first */
        raws    .resize(getCaptureFrameNum());
        rrzos   .resize(getCaptureFrameNum());
        gmvs    .resize(getCaptureFrameNum());
        rStatus .resize(getCaptureFrameNum());

        /* prepare output buffer */
        for (int i = 0; i < (int)getCaptureFrameNum(); i++) {
            raws[i] = &m_imgRaws[i].imgbuf;
            rrzos[i] = &m_imgRrzos[i].imgbuf;
            gmvs[i] = &m_globalMv[i];
            rStatus[i] = 0;
        }

        /* register event dispatcher and set MfllCore instance */
        m_spCapturer->setMfllCore((IMfllCore*)this);
        err = m_spCapturer->registerEventDispatcher(m_event);

        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: MfllCapture::registerEventDispatcher failed with code %d", __FUNCTION__, err);
            goto lbExit;
        }

        /* Catpure frames */
        mfllLogD3("Capture frames!");

        err = m_spCapturer->captureFrames(getCaptureFrameNum(), raws, rrzos, gmvs, rStatus);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: MfllCapture::captureFrames failed with code %d", __FUNCTION__, err);
            goto lbExit;
        }
        /* check if force set GMV to zero */
        if (CC_UNLIKELY(m_spProperty->getForceGmvZero() > 0)) {
            for (int i = 0; i < MFLL_MAX_FRAMES; i++) {
                m_globalMv[i].x = 0;
                m_globalMv[i].y = 0;
            }
        }
        else if (m_spProperty->getForceGmv(m_globalMv)) {
            mfllLogI("%s: force set Gmv as manual setting", __FUNCTION__);
            for (int i = 0; i < MFLL_MAX_FRAMES; i++) {
                mfllLogI("%s: m_globalMv[%d](x,y) = (%d, %d)", __FUNCTION__, i, m_globalMv[i].x, m_globalMv[i].y);
            }
        }


        // {{{ checks captured buffers
        /* check result, resort buffers, and update frame capture number and blend number if need */
        {
            std::vector<ImageBufferPack>     r; // raw buffer;
            std::vector<ImageBufferPack>     z; // rrzo buffer;
            std::vector<MfllMotionVector_t>  m; // motion

            r.resize(getCaptureFrameNum());
            z.resize(getCaptureFrameNum());
            m.resize(getCaptureFrameNum());

            size_t okCount = 0; // counting ok frame numbers

            /* check the status from Capturer, if status is 0 means ok */
            for (size_t i = 0, j = 0; i < (size_t)getCaptureFrameNum(); i++) {
                /* If not failed, save to stack */
                if (rStatus[i] == 0) {
                    r[j] = m_imgRaws[i];
                    z[j] = m_imgRrzos[i];
                    m[j] = m_globalMv[i];
                    okCount++;
                    j++;
                }
            }

            m_dbgInfoCore.frameCapture = okCount;
            m_dbgInfoCore.frameMaxCapture = getCaptureFrameNum();
            m_dbgInfoCore.frameMaxBlend = getBlendFrameNum();

            mfllLogD3("capture done, ok count=%zu, num of capture = %u",
                    okCount, getCaptureFrameNum());

            m_caputredCount = okCount;

            /* if not equals, something wrong */
            if (okCount != (size_t)getCaptureFrameNum()) {
                /* sort available buffers continuously */
                for (size_t i = 0; i < okCount; i++) {
                    m_imgRaws[i] = r[i];
                    m_imgRrzos[i] = z[i];
                    m_globalMv[i] = m[i];
                }

                /* boundary blending frame number */
                if (getBlendFrameNum() > okCount) {
                    m_dbgInfoCore.frameBlend = okCount;
                    /* by pass un-necessary operation due to no buffer, included the last frame */
                    for (size_t i = (okCount <= 0 ? 0 : okCount - 1); i < getBlendFrameNum(); i++) {
                        m_bypass.bypassMotionEstimation[i] = 1;
                        m_bypass.bypassMotionCompensation[i] = 1;
                        m_bypass.bypassBlending[i] = 1;
                        m_bypass.bypassEncodeQYuv[i] = 1;
                    }
                }

                /* if okCount <= 0, it means no source image can be used, ignore all operations */
                if (CC_UNLIKELY( okCount <= 0 )) {
                    mfllLogE("%s: okCount <= 0, no source image can be used, " \
                             "ignore all operations.(SF, BASEYUV, MIX)", __FUNCTION__);
                    m_bypass.bypassEncodeYuvBase = 1;
                    m_bypass.bypassEncodeYuvGolden = 1;
                    m_bypass.bypassMixing = 1;
                }
            }
        } // }}}
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_CaptureSingle(void *void_index)
{
    enum MfllErr err = MfllErr_Ok;

    const unsigned int index = (unsigned int)(long)void_index;

    if (CC_LIKELY(index < getCaptureFrameNum()))
    {
        /**
         *  N O T I C E
         *
         *  Aquire buffers should be invoked by MfllCapturer
         *  1. IMfllCore::doAllocRawBuffer
         *  2. IMfllCore::doAllocQyuvBuffer
         */
        /* check if IMfllCapturer has been assigned */
        if (CC_UNLIKELY(m_spCapturer.get() == NULL)) {
            mfllLogD3("%s: create MfllCapturer", __FUNCTION__);
            m_spCapturer = IMfllCapturer::createInstance();
            if (m_spCapturer.get() == NULL) {
                mfllLogE("%s: create MfllCapturer instance", __FUNCTION__);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
        }

        if (index == 0) {
            /* check if force set GMV to zero */
            if (CC_UNLIKELY(m_spProperty->getForceGmvZero() > 0)) {
                for (int i = 0; i < MFLL_MAX_FRAMES; i++) {
                    m_globalMv[i].x = 0;
                    m_globalMv[i].y = 0;
                }
                m_byPassGmv = true;
            }
            else if (m_spProperty->getForceGmv(m_globalMv)) {
                mfllLogI("%s: force set Gmv as manual setting", __FUNCTION__);
                for (int i = 0; i < MFLL_MAX_FRAMES; i++) {
                    mfllLogI("%s: m_globalMv[%d](x,y) = (%d, %d)", __FUNCTION__, i, m_globalMv[i].x, m_globalMv[i].y);
                }
                m_byPassGmv = true;
            }
             /* this case is yuv queue, we assume all input frame is Ok. */
            m_dbgInfoCore.frameCapture = m_frameCapture;
            m_dbgInfoCore.frameMaxCapture = getCaptureFrameNum();
            m_dbgInfoCore.frameMaxBlend = getBlendFrameNum();

            /* register event dispatcher and set MfllCore instance */
            m_spCapturer->setMfllCore((IMfllCore*)this);
            err = m_spCapturer->registerEventDispatcher(m_event);

            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: MfllCapture::registerEventDispatcher failed with code %d", __FUNCTION__, err);
                goto lbExit;
            }
        }

        sp<IMfllImageBuffer>*   yuvs;
        sp<IMfllImageBuffer>*   qyuvs;
        MfllMotionVector_t*     gmvs;
        MfllMotionVector_t      gmv_bypass;
        int                     rStatus;

        /* prepare output buffer */
        {
            yuvs    = &m_imgYuvs[index].imgbuf;
            qyuvs   = &m_imgQYuvs[index].imgbuf;
            gmvs    = (!m_byPassGmv)?&m_globalMv[index]:&gmv_bypass;
            rStatus = 0;
        }


        /* Catpure frames */
        mfllLogD3("Capture frames!");

        err = m_spCapturer->captureFrames(index, yuvs, qyuvs, gmvs, rStatus);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: MfllCapture::captureFrames failed with code %d", __FUNCTION__, err);
            goto lbExit;
        }

        /* check the status from Capturer, if status is 0 means ok */
        if (CC_UNLIKELY(rStatus != 0)) {
            mfllLogI("%s: MfllCapture::captureFrames failed with status %d", __FUNCTION__, rStatus);
        }
    }

lbExit:
    return err;
}

enum MfllErr MfllCore::do_Bss(intptr_t intptrframeCount)
{
    enum MfllErr err = MfllErr_Ok;
    int bssFrameCount = intptrframeCount;

    {
        sp<IMfllBss> bss = IMfllBss::createInstance();
        if (CC_UNLIKELY(bss.get() == NULL)) {
            mfllLogE("%s: create IMfllBss instance fail", __FUNCTION__);
            m_dbgInfoCore.bss_enable = 0;
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }
        bss->setMfllCore(this);

        err = bss->init(m_spNvramProvider);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: init BSS failed, ignore BSS", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            m_dbgInfoCore.bss_enable = 0;
            goto lbExit;
        }

        bss->setUniqueKey(m_uniqueKey);

        std::vector< sp<IMfllImageBuffer> > imgs;
        std::vector< sp<IMfllImageBuffer> > rrzos;
        std::vector< MfllMotionVector_t >   mvs;
        std::vector< int64_t > tss = m_timestampSync;

        for (int i = 0; i < bssFrameCount; i++) {
            imgs.push_back(m_imgRaws[i].imgbuf);
            rrzos.push_back(m_imgRrzos[i].imgbuf);
            mvs.push_back(m_globalMv[i]);
        }

        tss.resize(imgs.size(), -1);

        mfllLogD3("%s: RAW domain BSS", __FUNCTION__);
        std::vector<int> newIndex = bss->bss(rrzos, mvs, tss);
        if (newIndex.size() <= 0) {
            mfllLogE("%s: do bss failed", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        // sort items
        std::deque<ImageBufferPack> newRaws;
        std::deque<ImageBufferPack> newRrzos;
        for (int i = 0; i < bssFrameCount; i++) {
            int index = newIndex[i]; // new index
            newRaws.push_back(m_imgRaws[index]);
            newRrzos.push_back(m_imgRrzos[index]);
            m_bssIndex[i] = index;
            mfllLogD3("%s: new index (%u)-->(%d)", __FUNCTION__, i, index);
            /**
             *  mvs will be sorted by Bss, we don't need to re-sort it again,
             *  just update it
             */
            m_globalMv[i] = mvs[i];
        }
        for (int i = 0; i < bssFrameCount; i++) {
            m_imgRaws[i] = newRaws[i];
            m_imgRrzos[i] = newRrzos[i];
        }

        // check frame to skip
        size_t frameNumToSkip = bss->getSkipFrameCount();

        // due to adb force drop
        int forceDropNum   = MfllProperty::getDropNum();
        if (CC_UNLIKELY(forceDropNum > -1)) {
            mfllLogD("%s: force drop frame count = %d, original bss drop frame = %zu",
                    __FUNCTION__, forceDropNum, frameNumToSkip);

            frameNumToSkip = static_cast<size_t>(forceDropNum);
        }

        // due to capture M and blend N
        int frameNumToSkipBase = static_cast<int>(getCaptureFrameNum()) - static_cast<int>(getBlendFrameNum());
        if (CC_UNLIKELY(frameNumToSkip < frameNumToSkipBase)) {
            mfllLogD("%s: update drop frame count = %d, original drop frame = %zu",
                    __FUNCTION__, frameNumToSkipBase, frameNumToSkip);

            frameNumToSkip = static_cast<size_t>(frameNumToSkipBase);
        }

#if 0
        // test skip all frames
        frameNumToSkip = bssFrameCount - 1;
        // test out-of-range
        frameNumToSkip = 100;
#endif

        if (frameNumToSkip > 0) {
            // get the index of image buffers for releasing due to ignored
            int idxForImages =
                static_cast<int>(getCaptureFrameNum()) - static_cast<int>(frameNumToSkip);

            // get the infex of operations (including MEMC, blending) to ignore
            int idxForOperation = idxForImages - 1;
            if (idxForOperation < 0) {
                mfllLogE("%s: drop frame number(%zu) is more than capture number(%u), ignore all frames",
                        __FUNCTION__,
                        frameNumToSkip,
                        getCaptureFrameNum());
                idxForOperation = 0;
                idxForImages = 0;
            }
            mfllLogD("%s: skip frame count = %zu", __FUNCTION__, frameNumToSkip);

            if (idxForOperation <= 0) { // ignore all blending
                mfllLogW("%s: BSS drops all frames, using single capture", __FUNCTION__);
                // ignore encoding YUV base due to no need to blend
                m_bypass.bypassEncodeYuvBase = 1;
                m_bypass.bypassMixing = 1;
                m_bAsSingleFrame = 1;
                if (m_bDoDownscale && m_downscaleDivisor > 0) { // if support downscale
                    if (m_bDoDownscale == false) {
                        m_width = m_width*m_downscaleDividend/m_downscaleDivisor;
                        m_height = m_height*m_downscaleDividend/m_downscaleDivisor;

                        /* update debug info */
                        m_dbgInfoCore.blend_yuv_width = m_width;
                        m_dbgInfoCore.blend_yuv_height = m_height;
                    }
                    m_bDoDownscale = true;
                    mfllLogI("%s: use downscale yuv size(%u, %u) to blend for single capture", __FUNCTION__, m_width, m_height);
                }
            }

            for (size_t i = static_cast<size_t>(idxForOperation); i < MFLL_MAX_FRAMES; i++) {
                if (i >= static_cast<size_t>(idxForOperation)) {
                    mfllLogD("%s: bypass MEMC/BLD (index = %zu)", __FUNCTION__, i);
                    m_bypass.bypassMotionEstimation[i] = 1;
                    m_bypass.bypassMotionCompensation[i] = 1;
                    m_bypass.bypassBlending[i] = 1;
                }
            }

            for (size_t i = static_cast<size_t>(idxForImages); i < MFLL_MAX_FRAMES; i++) {
                if (i >= static_cast<size_t>(idxForImages)) {
                    mfllLogD("%s: bypass encodeRawToYuv (index = %zu)", __FUNCTION__, i);
                    m_bypass.bypassEncodeQYuv[i] = 1;
                }
            }
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_EncodeQYuv(void *void_index)
{
    enum MfllErr err = MfllErr_Ok;

    const unsigned int index = (unsigned int)(long)void_index;

    if (CC_UNLIKELY(index >= MFLL_MAX_FRAMES)){
        mfllLogE("index:%u is out of frames index:%d, ignore it", index, MFLL_MAX_FRAMES);
        return MfllErr_BadArgument;
    }
    else {
        /* handle RAW to YUV frames */
        if ( m_imgRaws[index].imgbuf.get() ) {
            mfllLogD3("Convert RAW to YUV");

            // check if full yuv exists
            bool bFullYuv = m_imgYuvs[index].imgbuf.get() ? true : false;

            /* capture YUV */
            /* no raw or ignored */
            err = doAllocQyuvBuffer((void*)(long)index);

            // check if buffers are ready to use && qyuvs[index] exists
            bool bBuffersReady = ( (err == MfllErr_Ok) && m_imgQYuvs[index].imgbuf.get() );

            if (CC_UNLIKELY( ! bBuffersReady )) {
                mfllLogE("%s: alloc QYUV buffer %u failed", __FUNCTION__, index);
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

                if (bFullYuv) {
                    mfllLogD3("%s: RAW to 2 YUVs (index = %u), %dx%d, %dx%d.", __FUNCTION__, index,
                            m_imgYuvs[index].imgbuf->getWidth(), m_imgYuvs[index].imgbuf->getHeight(),
                            m_imgQYuvs[index].imgbuf->getWidth(), m_imgQYuvs[index].imgbuf->getHeight());

                    /**
                     *  if we've already known that it's single capture, we don't need
                     *  stage BFBLD because we use stage SF to generate the output.
                     */
                    if ( getCaptureFrameNum() > 1) {
                        if (retrieveBuffer(MfllBuffer_PostviewYuv).get()) {
                            if (m_bDoDownscale && m_downscaleDivisor > 0) {
                                err = m_spMfb->encodeRawToYuv(
                                    m_imgRaws[index].imgbuf.get(),  // source
                                    m_imgYuvs[index].imgbuf.get(),  // output
                                    m_imgQYuvs[index].imgbuf.get(), // output2
                                    MfllRect_t(), // crop for output, no need cropping
                                    MfllRect_t(), // crop for output2, no need cropping
                                    YuvStage_RawToYv16
                                    );

                                MfllRect_t postviewCropRgn = m_postviewCropRgn;
                                mfllLogD("%s: enable downscale, crop region before downscale(x:%d, y:%d, w:%d, h:%d)",
                                        __FUNCTION__, m_postviewCropRgn.x, m_postviewCropRgn.y, m_postviewCropRgn.w, m_postviewCropRgn.h);
                                postviewCropRgn.x = m_postviewCropRgn.x*m_downscaleDividend/m_downscaleDivisor;
                                postviewCropRgn.y = m_postviewCropRgn.y*m_downscaleDividend/m_downscaleDivisor;
                                postviewCropRgn.w = m_postviewCropRgn.w*m_downscaleDividend/m_downscaleDivisor;
                                postviewCropRgn.h = m_postviewCropRgn.h*m_downscaleDividend/m_downscaleDivisor;
                                mfllLogD("%s: enable downscale, crop region after downscale(x:%d, y:%d, w:%d, h:%d)",
                                        __FUNCTION__, postviewCropRgn.x, postviewCropRgn.y, postviewCropRgn.w, postviewCropRgn.h);

                                /* generate postview by MDP */
                                err = m_spMfb->convertYuvFormatByMdp(
                                    m_imgYuvs[index].imgbuf.get(),  // input
                                    retrieveBuffer(MfllBuffer_PostviewYuv).get(), // output1
                                    NULL, // output2
                                    postviewCropRgn, // crop for output1
                                    MfllRect_t(), // crop for output2, no need cropping
                                    YuvStage_Unknown
                                    );
                            }
                            else {
                                err = m_spMfb->encodeRawToYuv(
                                    m_imgRaws[index].imgbuf.get(),  // source
                                    m_imgYuvs[index].imgbuf.get(),  // output
                                    m_imgQYuvs[index].imgbuf.get(), // output2
                                    retrieveBuffer(MfllBuffer_PostviewYuv).get(), // output3
                                    MfllRect_t(), // crop for output2, no need cropping
                                    m_postviewCropRgn, // crop for output3
                                    YuvStage_RawToYv16
                                    );
                            }
                        }
                        else {
                            err = m_spMfb->encodeRawToYuv(
                                m_imgRaws[index].imgbuf.get(),
                                m_imgYuvs[index].imgbuf.get(),
                                m_imgQYuvs[index].imgbuf.get(),
                                MfllRect_t(), // no need cropping
                                MfllRect_t(), // no need cropping
                                YuvStage_RawToYv16
                                );
                        }
                    }

                    if (CC_UNLIKELY(err != MfllErr_Ok)) {
                        mfllLogE("%s: Encode RAW to YUV fail", __FUNCTION__);
                        goto lbExit;
                    }
                }
                else {
                    /**
                     *  if we've already known that it's single capture, we don't need
                     *  stage BFBLD because we use stage SF to generate the output.
                     */
                    if ( getCaptureFrameNum() > 1) {
                        err = m_spMfb->encodeRawToYuv(
                            m_imgRaws[index].imgbuf.get(),
                            m_imgQYuvs[index].imgbuf.get(),
                            retrieveBuffer(MfllBuffer_PostviewYuv).get(),
                            MfllRect_t(),
                            m_postviewCropRgn,
                            YuvStage_RawToYv16);
                    }

                    if (CC_UNLIKELY(err != MfllErr_Ok)) {
                        mfllLogE("%s: Encode RAW to YUV fail", __FUNCTION__);
                        goto lbExit;
                    }
                }
            }
        }
    }

lbExit:
    if (CC_UNLIKELY(err != MfllErr_Ok)) {
        mfllLogD("%s: bypass MEMC/BLD (index = %u)", __FUNCTION__, index);
        m_bypass.bypassEncodeMss[index] = 1;
        m_bypass.bypassMotionEstimation[index] = 1;
        m_bypass.bypassMotionCompensation[index] = 1;
        m_bypass.bypassBlending[index] = 1;
    }

    // clear image
    if (index != 0) {
        mfllLogD3("%s: clear images (index = %u)", __FUNCTION__, index);

        // release RAW image buffer
        m_imgRrzos[index].releaseBufLocked();
        m_imgRaws[index].releaseBufLocked();
    }
    return err;
}


enum MfllErr MfllCore::do_EncodeMss(void * /*void_index*/)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);
    MfllOperationSync::getInstance()->addJob(MfllOperationSync::JOB_MSS);

    return MfllErr_NotImplemented;
}

enum MfllErr MfllCore::do_Msf(void * /*void_index*/)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);
    MfllOperationSync::getInstance()->addJob(MfllOperationSync::JOB_MSF);

    return MfllErr_NotImplemented;
}

enum MfllErr MfllCore::do_EncodeYuvBase(JOB_VOID)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}

enum MfllErr MfllCore::do_EncodeYuvGoldenDownscale(JOB_VOID)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}

enum MfllErr MfllCore::do_EncodeYuvGolden(JOB_VOID)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}


enum MfllErr MfllCore::do_MotionEstimation(void *void_index)
{
    unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;
    std::string _log = std::string("start ME") + to_char(index);

    {
        mfllAutoLog(_log.c_str());
        unsigned int memcIndex = index % getMemcInstanceNum();
        sp<IMfllMemc> memc = m_spMemc[memcIndex];
        if (CC_UNLIKELY(memc.get() == NULL)) {
            mfllLogE("%s: MfllMemc is necessary to be created first (index=%u)", __FUNCTION__, index);
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


enum MfllErr MfllCore::do_MotionCompensation(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;

    /* using full size YUV if is using full size MC */
    ImageBufferPack* pMcRef = (m_isFullSizeMc == 0)
        ? &m_imgQYuvs[index + 1]
        : &m_imgYuvs[index + 1]
        ;

    {
        unsigned int memcIndex = index % getMemcInstanceNum();
        sp<IMfllMemc> memc = m_spMemc[memcIndex];

        if (CC_UNLIKELY(memc.get() == NULL)) {
            mfllLogE("%s: MfllMemc is necessary to be created first (index=%u)", __FUNCTION__, index);
            err = MfllErr_NullPointer;
            goto lbExit;
        }

        /* check if we need to do MC or not */
        if (memc->isIgnoredMotionCompensation()) {
            mfllLogD3("%s: ignored motion compensation & blending", __FUNCTION__);
            m_bypass.bypassBlending[index] = 1;
            goto lbExit;
        }

        /* allocate YUV MC working buffer if necessary */
        err = doAllocYuvMcWorking(NULL);
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: allocate YUV MC working buffer failed", __FUNCTION__);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        /* setting up mc */
        memc->setMcRefImage(pMcRef->imgbuf);
        memc->setMcDstImage(m_imgYuvMcWorking.imgbuf);

        err = memc->motionCompensation();
        if (CC_UNLIKELY(err != MfllErr_Ok)) {
            mfllLogE("%s: IMfllMemc::motionCompensation failed, returns %d", __FUNCTION__, (int)err);
            err = MfllErr_UnexpectedError;
            goto lbExit;
        }

        /* exchange buffer from Dst->Src */
        {
            ImageBufferPack::swap(pMcRef, &m_imgYuvMcWorking);
            /* convert image format without alignment for blending or mixing */
            pMcRef->imgbuf->setAligned(16, 16);
            pMcRef->imgbuf->convertImageFormat(MFLL_IMAGE_FORMAT_YUV_MC_WORKING);
        }

        /* sync CPU cache to HW */
        pMcRef->imgbuf->syncCache(); // CPU->HW
    }

lbExit:
    return err;
}

enum MfllErr MfllCore::do_AllocRawBuffer(void *void_index)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}


enum MfllErr MfllCore::do_AllocRrzoBuffer(void *void_index)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}


enum MfllErr MfllCore::do_AllocQyuvBuffer(void *void_index)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}

enum MfllErr MfllCore::do_AllocMssBuffer(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;

    size_t start = (index)?0:2;
    unsigned int width  = m_imgoWidth;
    unsigned int height = m_height;

    m_imgMssYuvs[index].setSize(MFLL_PYRAMID_SIZE);

    for (size_t i = 0 ; i < m_imgMssYuvs[index].getSize() ; i++) {
        //update width and height: S(n+1) = (S(n) + 3)/4*2
        if (i > 0) {
            width = (width+3)/4*2;
            height = (height+3)/4*2;
        }
        if (i < start)
            continue;
        std::lock_guard<std::mutex> __l(m_imgMssYuvs[index].images[i].locker);

        /* create IMfllImageBuffer instances */
        sp<IMfllImageBuffer> pImg = m_imgMssYuvs[index].images[i].imgbuf;
        if (pImg == NULL) {
            pImg = IMfllImageBuffer::createInstance(PYRAMID_NAME("mss_", index, i));
            if (CC_UNLIKELY(pImg == NULL)) {
                mfllLogE("%s: create IMfllImageBuffer(%u.%zu) failed", __FUNCTION__, index, i);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
            m_imgMssYuvs[index].images[i].imgbuf = pImg;
        }
        auto imgFormat = (i+1 != m_imgMssYuvs[index].getSize())?MFLL_IMAGE_FORMAT_MSS:MFLL_IMAGE_FORMAT_MSS_SN;
        /* if not init, init it */
        if (!pImg->isInited()) {
            pImg->setImageFormat(imgFormat);
            pImg->setResolution(width, height);
            pImg->setAligned(16, 16);
            err = pImg->initBuffer();
            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: init MSS buffer(%u.%zu) failed", __FUNCTION__, index, i);
                err = MfllErr_UnexpectedError;
                goto lbExit;
            }
            /* convert image size to real width and height */
            pImg->setResolution(width, height);
            pImg->setAligned(16, 16);
            pImg->convertImageFormat(imgFormat);
        }
        else {
            mfllLogD3("%s: MSS buffer %u.%zu is inited, ignore here", __FUNCTION__, index, i);
            /* convert image size to real width and height */
            pImg->setResolution(width, height);
            pImg->setAligned(16, 16);
            pImg->convertImageFormat(imgFormat);
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_AllocYuvBase(JOB_VOID)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}


enum MfllErr MfllCore::do_AllocYuvGolden(JOB_VOID)
{
    mfllLogE("%s: MfllErr_NotImplemented", __FUNCTION__);

    return MfllErr_NotImplemented;
}


enum MfllErr MfllCore::do_AllocYuvWorking(JOB_VOID)
{
    enum MfllErr err = MfllErr_Ok;

    for (int index = 0 ; index < 3 ; index++) {
        unsigned int width  = m_imgoWidth;
        unsigned int height = m_height;

        m_imgMsfBlended[index].setSize(MFLL_PYRAMID_SIZE);

        for (size_t i = 0 ; i < m_imgMsfBlended[index].getSize() ; i++) {
            //update width and height: S(n+1) = (S(n) + 3)/4*2
            if (i > 0) {
                width = (width+3)/4*2;
                height = (height+3)/4*2;
            }
            //output s0 is set from p2a
            if (i == 0 && index == 2)
                continue;
            std::lock_guard<std::mutex> __l(m_imgMsfBlended[index].images[i].locker);
            sp<IMfllImageBuffer> pImg = m_imgMsfBlended[index].images[i].imgbuf;
            if (CC_UNLIKELY(pImg == NULL)) {
                pImg = IMfllImageBuffer::createInstance(BUFFER_NAME("yuv_blended", i));
                if (pImg == NULL) {
                    mfllLogE("%s: create YUV blended buffer instance failed", __FUNCTION__);
                    err = MfllErr_CreateInstanceFailed;
                    goto lbExit;
                }
                m_imgMsfBlended[index].images[i].imgbuf = pImg;
            }
            auto mssFormat = (i+1 != m_imgMsfBlended[index].getSize())?MFLL_IMAGE_FORMAT_MSS:MFLL_IMAGE_FORMAT_MSS_SN;
            auto imgFormat = (index == 2)? MFLL_IMAGE_FORMAT_MSF_DL : mssFormat;
            if (!pImg->isInited()) {
                pImg->setImageFormat(imgFormat);
                pImg->setResolution(width, height);
                pImg->setAligned(16, 16);
                err = pImg->initBuffer();
                if (CC_UNLIKELY(err != MfllErr_Ok)) {
                    mfllLogE("%s: init YUV blended buffer failed", __FUNCTION__);
                    err = MfllErr_UnexpectedError;
                    goto lbExit;
                }
                /* convert image size to real width and height */
                pImg->setResolution(width, height);
                pImg->setAligned(16, 16);
                pImg->convertImageFormat(imgFormat);
            }
            else {
                mfllLogD3("%s: YUV YUV blended buffer re-init(%dx%d)", __FUNCTION__, width, height);
                pImg->setResolution(width, height);
                pImg->setAligned(16, 16);
                pImg->convertImageFormat(imgFormat);
            }
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_AllocYuvMcWorking(JOB_VOID)
{
    enum MfllErr err = MfllErr_Ok;

    {
        std::lock_guard<std::mutex> __l(m_imgYuvMcWorking.locker);

        sp<IMfllImageBuffer> pImg = m_imgYuvMcWorking.imgbuf;

        if (pImg == NULL) {
            pImg = IMfllImageBuffer::createInstance("yuv_mc_working", m_isFullSizeMc ? Flag_FullSize : Flag_QuarterSize);
            if (pImg == NULL) {
                mfllLogE("%s: create YUV MC working buffer instance failed", __FUNCTION__);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
            m_imgYuvMcWorking.imgbuf = pImg;
        }

        if (!pImg->isInited()) {
            pImg->setImageFormat(MFLL_IMAGE_FORMAT_YUV_MC_WORKING);
            if (m_isFullSizeMc) {
                pImg->setResolution(m_imgoWidth, m_imgoHeight);
            }
            else
                pImg->setResolution(m_qwidth, m_qheight);
            pImg->setAligned(16, 16);
            err = pImg->initBuffer();
            if (err != MfllErr_Ok) {
                mfllLogE("%s: init YUV MC working buffer failed", __FUNCTION__);
                err = MfllErr_UnexpectedError;
                goto lbExit;
            }
            /* convert image buffer to real size */
            pImg->setResolution(
                    (m_isFullSizeMc ? m_width  : m_qwidth),
                    (m_isFullSizeMc ? m_height : m_qheight)
                    );
            pImg->setAligned(16, 16);
            pImg->convertImageFormat(MFLL_IMAGE_FORMAT_YUV_MC_WORKING);
        }
        else {
            mfllLogD3("%s: YUV MC working buffer re-init(%dx%d)", __FUNCTION__, m_width, m_height);
            if (m_isFullSizeMc) {
                pImg->setResolution(m_width, m_height);
            }
            else {
                pImg->setResolution(m_qwidth, m_qheight);
            }
            pImg->setAligned(16, 16);
            pImg->convertImageFormat(MFLL_IMAGE_FORMAT_YUV_MC_WORKING);
        }
    }

lbExit:
    return err;
}


enum MfllErr MfllCore::do_AllocWeighting(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;

    enum MfllErr err = MfllErr_Ok;


    unsigned int width  = m_imgoWidth;
    unsigned int height = m_height;

    m_imgWeightings[index].setSize(MFLL_PYRAMID_SIZE-1);

    for (size_t i = 0 ; i < m_imgWeightings[index].getSize() ; i++) {
        //update width and height: S(n+1) = (S(n) + 3)/4*2
        if (i > 0) {
            width = (width+3)/4*2;
            height = (height+3)/4*2;
        }
        std::lock_guard<std::mutex> __l(m_imgWeightings[index].images[i].locker);

        sp<IMfllImageBuffer> pImg = m_imgWeightings[index].images[i].imgbuf;
        if (CC_UNLIKELY(pImg == NULL)) {
            pImg = IMfllImageBuffer::createInstance(PYRAMID_NAME("wt_", index, i)/*, Flag_WeightingTable*/);
            if (pImg == NULL) {
                mfllLogE("%s: create weighting table(%u) buffer instance failed", __FUNCTION__, index);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
            m_imgWeightings[index].images[i].imgbuf = pImg;

            if (!pImg->isInited()) {
                pImg->setImageFormat(MFLL_IMAGE_FORMAT_WEIGHTING);
                pImg->setResolution(width, height);
                pImg->setAligned(16, 16);
                err = pImg->initBuffer();
                if (CC_UNLIKELY(err != MfllErr_Ok)) {
                    mfllLogE("%s: init weighting table(%u) buffer failed", __FUNCTION__, index);
                    err = MfllErr_UnexpectedError;
                    goto lbExit;
                }
            }
            else {
                mfllLogD3("%s: YUV weighting table(%u) buffer re-init(%dx%d)", __FUNCTION__, index, width, height);
                pImg->setResolution(width, height);
                pImg->setAligned(16, 16);
                pImg->convertImageFormat(MFLL_IMAGE_FORMAT_WEIGHTING);
            }
        }
    }
lbExit:
    return err;
}


enum MfllErr MfllCore::do_AllocMemcWorking(void *void_index)
{
    const unsigned int index = (unsigned int)(long)void_index;
    enum MfllErr err = MfllErr_Ok;

    {
        int bufferIndex = index % getMemcInstanceNum();

        std::lock_guard<std::mutex> __l(m_imgMemc[bufferIndex].locker);

        sp<IMfllImageBuffer> pImg = m_imgMemc[bufferIndex].imgbuf;

        if (CC_UNLIKELY(pImg == NULL)) {
            pImg = IMfllImageBuffer::createInstance(BUFFER_NAME("memc_", bufferIndex), Flag_Algorithm);
            if (pImg == NULL) {
                mfllLogE("%s: create MEMC working buffer(%d) instance failed", __FUNCTION__, bufferIndex);
                err = MfllErr_CreateInstanceFailed;
                goto lbExit;
            }
            m_imgMemc[bufferIndex].imgbuf = pImg;
        }

        if (!pImg->isInited()) {
            sp<IMfllMemc> memc = m_spMemc[bufferIndex];
            if (CC_UNLIKELY(memc.get() == NULL)) {
                mfllLogE("%s: memc instance(index %d) is NULL", __FUNCTION__, bufferIndex);
                err = MfllErr_NullPointer;
                goto lbExit;
            }
            size_t bufferSize = 0;
            err = memc->getAlgorithmWorkBufferSize(&bufferSize);
            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: get algorithm working buffer size fail", __FUNCTION__);
                goto lbExit;
            }

            /* check if 2 bytes alignment */
            if (bufferSize % 2 != 0) {
                mfllLogW("%s: algorithm working buffer size is not 2 bytes alignment, make it is", __FUNCTION__);
                bufferSize += 1;
            }

            pImg->setImageFormat(MFLL_IMAGE_FORMAT_MEMC_WORKING);
            pImg->setAligned(2, 2); // always using 2 bytes align
            pImg->setResolution(bufferSize/2, 2);
            err = pImg->initBuffer();
            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: init MEMC working buffer(%d) failed", __FUNCTION__, bufferIndex);
                err = MfllErr_UnexpectedError;
                goto lbExit;
            }
        }
        else {
            sp<IMfllMemc> memc = m_spMemc[bufferIndex];
            if (CC_UNLIKELY(memc.get() == NULL)) {
                mfllLogE("%s: memc instance(index %d) is NULL", __FUNCTION__, bufferIndex);
                err = MfllErr_NullPointer;
                goto lbExit;
            }
            size_t bufferSize = 0;
            err = memc->getAlgorithmWorkBufferSize(&bufferSize);
            if (CC_UNLIKELY(err != MfllErr_Ok)) {
                mfllLogE("%s: get algorithm working(%d) buffer size fail", __FUNCTION__, bufferIndex);
                goto lbExit;
            }
            mfllLogD3("%s: MEMC working buffer %d re-init(%zux%d)", __FUNCTION__, bufferIndex, bufferSize/2, 2);
            pImg->setAligned(2, 2); // always using 2 bytes align
            pImg->setResolution(bufferSize/2, 2);
            pImg->convertImageFormat(MFLL_IMAGE_FORMAT_MEMC_WORKING);
        }
    }

lbExit:
    return err;
}


}; // namespace mfll
#endif//__MFLLCORE_IMPL_INC__
