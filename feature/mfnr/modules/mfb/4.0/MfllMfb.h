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
#ifndef __MfllMfb_H__
#define __MfllMfb_H__

#include <mtkcam3/feature/mfnr/IMfllMfb.h>

#define LOG_TAG "MfllCore/Mfb"
#include <mtkcam3/feature/mfnr/MfllLog.h>
// MTKCAM
#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IEgnStream.h>
#include <mtkcam/drv/def/mfbcommon_v20.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>

#include <mtkcam/aaa/IHalISP.h>

// CUSTOM (platform)
#include <isp_tuning/isp_tuning.h>
using namespace NSIspTuning;

// AOSP
#include <utils/Mutex.h> // android::Mutex

// STL
#include <functional> // std::function
#include <memory> // std::unique_ptr
#include <deque> // std::deque
#include <unordered_map> // std::unordered_map
#include <cstdint> // intptr_t
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "MfllIspProfiles.h"


using android::sp;
using android::Mutex;
using NSCam::IImageBuffer;
using NSCam::MRect;
using NSCam::MSize;
using NSCam::MPoint;
using NS3Av3::MetaSet_T;
using NSCam::NSIoPipe::NSPostProc::INormalStream;
using NS3Av3::IHalISP;
using NS3Av3::TuningParam;
using NSCam::IMetadata;
using namespace NSCam::TuningUtils;


/* MFB new APIs */
using NSCam::NSIoPipe::NSMFB20::MSFConfig;
using NSCam::NSIoPipe::NSMFB20::MSSConfig;
using NSCam::NSIoPipe::NSEgn::IEgnStream;
using NSCam::NSIoPipe::NSEgn::EGNParams;


namespace mfll {

/* P2 callback Cookie typedef */
namespace MfllMfb_Imp {
#define CHECKSUM_BUFSIZE 4

    constexpr const uint32_t COOKIE_CHECKSUM1 = 0xC00C1E01;
    constexpr const uint32_t COOKIE_CHECKSUM2 = 0xC00C1E02;
    constexpr const uint32_t COOKIE_CHECKSUM3 = 0xC00C1E03;
    constexpr const uint32_t COOKIE_CHECKSUM4 = 0xC00C1E04;

    struct P2Cookie
    {
        // we do not use atomic or volatile to sepcify checksum because we want to
        // check visible side-effect problem too.
        uint32_t                    __checkBuffer1[CHECKSUM_BUFSIZE][4];
        uint32_t                    __checksum1;
        std::mutex                  __mtx;
        uint32_t                    __checksum2;
        std::condition_variable     __cv;
        uint32_t                    __checksum3;
        std::atomic<int>            __signaled;
        uint32_t                    __checksum4;
        uint32_t                    __checkBuffer2[CHECKSUM_BUFSIZE][4];
        //
        P2Cookie() : __checkBuffer1{}, __checkBuffer2{}
        {
            __checksum1 = COOKIE_CHECKSUM1;
            __checksum2 = COOKIE_CHECKSUM2;
            __checksum3 = COOKIE_CHECKSUM3;
            __checksum4 = COOKIE_CHECKSUM4;
            __signaled.store(0, std::memory_order_seq_cst);
        };
    };

    inline void checkSumDump(const P2Cookie& c)
    {
        for (int i = 0 ; i < CHECKSUM_BUFSIZE ; i++)
            mfllLogI("%s: __checkBuffer1[%d]  %#x  %#x  %#x  %#x",
                __FUNCTION__, i, c.__checkBuffer1[i][0], c.__checkBuffer1[i][1], c.__checkBuffer1[i][2], c.__checkBuffer1[i][3]);
        //
        mfllLogI("%s: __checksum1=%#x, __checksum2=%#x, __checksum3=%#x, __checksum4=%#x",
            __FUNCTION__, c.__checksum1, c.__checksum2, c.__checksum3, c.__checksum4);
        //
        for (int i = 0 ; i < CHECKSUM_BUFSIZE ; i++)
            mfllLogI("%s: __checkBuffer2[%d]  %#x  %#x  %#x  %#x",
                __FUNCTION__, i, c.__checkBuffer2[i][0], c.__checkBuffer2[i][1], c.__checkBuffer2[i][2], c.__checkBuffer2[i][3]);
    }

    inline bool isValidCookie(const P2Cookie& c)
    {
        if (__builtin_expect( c.__checksum1 != COOKIE_CHECKSUM1, false)) {
            mfllLogE("checksum doesn't match, expected,real=%#x, %#x",
                    COOKIE_CHECKSUM1, c.__checksum1);
            return false;
        }
        if (__builtin_expect( c.__checksum2 != COOKIE_CHECKSUM2, false)) {
            mfllLogE("checksum doesn't match, expected,real=%#x, %#x",
                    COOKIE_CHECKSUM2, c.__checksum2);
            return false;
        }
        if (__builtin_expect( c.__checksum3 != COOKIE_CHECKSUM3, false)) {
            mfllLogE("checksum doesn't match, expected,real=%#x, %#x",
                    COOKIE_CHECKSUM3, c.__checksum3);
            return false;
        }
        if (__builtin_expect( c.__checksum4 != COOKIE_CHECKSUM4, false)) {
            mfllLogE("checksum doesn't match, expected,real=%#x, %#x",
                    COOKIE_CHECKSUM4, c.__checksum4);
            return false;
        }
        return true;
    }

#undef CHECKSUM_BUFSIZE
};

class MfllMfb : public IMfllMfb {
public:
    MfllMfb(void);
    virtual ~MfllMfb(void);

/* implementations of IMfllMfb */
public:
    virtual enum MfllErr            init(
                                        int sensorId
                                    ) override;


    virtual void                    setMfllCore(
                                        IMfllCore *c
                                    ) override;


    virtual void                    setShotMode(
                                        const enum MfllMode& mode
                                    ) override;


    virtual void                    setPostNrType(
                                        const enum NoiseReductionType &type
                                    ) override;


    virtual enum MfllErr            blend(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* out,
                                        IMfllImageBuffer* wt_in,
                                        IMfllImageBuffer* wt_out
                                    ) override;


    virtual enum MfllErr            blend(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* conf,
                                        IMfllImageBuffer* out,
                                        IMfllImageBuffer* wt_in,
                                        IMfllImageBuffer* wt_out
                                    ) override;

    virtual enum MfllErr            blend(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* conf,
                                        IMfllImageBuffer* mcmv,
                                        IMfllImageBuffer* out,
                                        IMfllImageBuffer* wt_in,
                                        IMfllImageBuffer* wt_out
                                    ) override;

    virtual enum MfllErr            mix(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* out,
                                        IMfllImageBuffer* wt
                                    ) override;

    virtual enum MfllErr            mix(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* out,
                                        IMfllImageBuffer* wt,
                                        IMfllImageBuffer* dceso
                                    ) override;


    virtual enum MfllErr            mix(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* out_main,
                                        IMfllImageBuffer* out_thumbnail,
                                        IMfllImageBuffer* wt,
                                        const MfllRect_t& output_thumb_crop
                                    ) override;

    virtual enum MfllErr            mix(
                                        IMfllImageBuffer* base,
                                        IMfllImageBuffer* ref,
                                        IMfllImageBuffer* out_main,
                                        IMfllImageBuffer* out_thumbnail,
                                        IMfllImageBuffer* wt,
                                        IMfllImageBuffer* dceso,
                                        const MfllRect_t& output_thumb_crop
                                    ) override;


    virtual enum MfllErr            setSyncPrivateData(
                                        const std::deque<void*>& dataset
                                    ) override;


    virtual enum MfllErr            setMixDebug(
                                        sp<IMfllImageBuffer> buffer
                                    ) override;


    virtual enum MfllErr            encodeRawToYuv(
                                        IMfllImageBuffer*       input,
                                        IMfllImageBuffer*       output,
                                        const enum YuvStage&    s
                                    ) override;


    virtual enum MfllErr            encodeRawToYuv(
                                        IMfllImageBuffer*       input,
                                        IMfllImageBuffer*       output,
                                        IMfllImageBuffer*       output_q,
                                        const MfllRect_t&       output_crop,
                                        const MfllRect_t&       output_q_crop,
                                        enum YuvStage           s = YuvStage_RawToYuy2
                                    ) override;


    virtual enum MfllErr            encodeRawToYuv(
                                        IMfllImageBuffer*       input,
                                        IMfllImageBuffer*       output,
                                        IMfllImageBuffer*       output2,
                                        IMfllImageBuffer*       output3,
                                        const MfllRect_t&       output2_crop,
                                        const MfllRect_t&       output3_crop,
                                        enum YuvStage           s = YuvStage_RawToYuy2
                                    ) override;

    virtual enum MfllErr            encodeRawToYuv(
                                        IMfllImageBuffer*       input,
                                        IMfllImageBuffer*       output,
                                        IMfllImageBuffer*       output2,
                                        IMfllImageBuffer*       output3,
                                        IMfllImageBuffer*       decso,
                                        const MfllRect_t&       output2_crop,
                                        const MfllRect_t&       output3_crop,
                                        enum YuvStage           s = YuvStage_RawToYuy2
                                    ) override;

    virtual enum MfllErr            mss(
                                        vector<IMfllImageBuffer *>& pylamid,
                                        IMfllImageBuffer*       omc,
                                        IMfllImageBuffer*       mcmv,
                                        IMfllImageBuffer*       dceso,
                                        int                     index,
                                        enum YuvStage           s = YuvStage_Mss
                                    ) override;

    virtual enum MfllErr            msf(
                                        vector<IMfllImageBuffer *>& base,
                                        vector<IMfllImageBuffer *>& ref,
                                        vector<IMfllImageBuffer *>& out,
                                        vector<IMfllImageBuffer *>& wt_in,
                                        vector<IMfllImageBuffer *>& wt_out,
                                        vector<IMfllImageBuffer *>& wt_ds,
                                        IMfllImageBuffer*       difference,
                                        IMfllImageBuffer*       conf,
                                        IMfllImageBuffer*       dsFrame,
                                        IMfllImageBuffer*       dceso,
                                        int                     index,
                                        enum YuvStage           s = YuvStage_Msf
                                    ) override;

    virtual enum MfllErr            convertYuvFormatByMdp(
                                        IMfllImageBuffer*       input,
                                        IMfllImageBuffer*       output,
                                        IMfllImageBuffer*       output_q,
                                        const MfllRect_t&       output_crop,
                                        const MfllRect_t&       output_q_crop,
                                        enum YuvStage           s = YuvStage_Unknown
                                    ) override;


/* private interface */
protected:
    enum MfllErr                    encodeRawToYuv(
                                        IImageBuffer*           src,
                                        IImageBuffer*           dst,
                                        IImageBuffer*           dst2,
                                        IImageBuffer*           dst3,
                                        IImageBuffer*           dceso,
                                        const MfllRect_t&       output_crop,
                                        const MfllRect_t&       output_q_crop,
                                        const MfllRect_t&       output_3_crop,
                                        const enum YuvStage&    s
                                    );


    enum MfllErr                    convertYuvFormatByMdp(
                                        IImageBuffer*           src,
                                        IImageBuffer*           dst,
                                        IImageBuffer*           dst2,
                                        const MfllRect_t&       output_crop,
                                        const MfllRect_t&       output_q_crop,
                                        const enum YuvStage     s
                                    );


    enum MfllErr                    blend(
                                        IImageBuffer*   base,
                                        IImageBuffer*   ref,
                                        IImageBuffer*   out,
                                        IImageBuffer*   wt_in,
                                        IImageBuffer*   wt_out,
                                        IImageBuffer*   confmap = nullptr,
                                        IImageBuffer*   memc = nullptr
                                    );

    virtual enum MfllErr            mss(
                                        vector<IImageBuffer *>& pylamid,
                                        IImageBuffer*           omc,
                                        IImageBuffer*           mcmv,
                                        IImageBuffer*           dceso,
                                        int                     index,
                                        const enum YuvStage     s = YuvStage_Mss
                                    );

    virtual enum MfllErr            msf(
                                        vector<IImageBuffer *>& base,
                                        vector<IImageBuffer *>& ref,
                                        vector<IImageBuffer *>& out,
                                        vector<IImageBuffer *>& wt_in,
                                        vector<IImageBuffer *>& wt_out,
                                        vector<IImageBuffer *>& wt_ds,
                                        IImageBuffer*           difference,
                                        IImageBuffer*           conf,
                                        IImageBuffer*           dsFrame,
                                        IImageBuffer*           dceso,
                                        int                     index,
                                        const enum YuvStage     s = YuvStage_Msf
                                    );

    virtual enum MfllErr            msf_p2(
                                        vector<IImageBuffer *>& base,
                                        vector<IImageBuffer *>& ref,
                                        vector<IImageBuffer *>& out,
                                        vector<IImageBuffer *>& wt_in,
                                        vector<IImageBuffer *>& wt_out,
                                        vector<IImageBuffer *>& wt_ds,
                                        IImageBuffer*           difference,
                                        IImageBuffer*           conf,
                                        IImageBuffer*           dsFrame,
                                        IImageBuffer*           dceso,
                                        int                     index,
                                        const enum YuvStage     s
                                    );

    enum MfllErr                    generateMssParam
                                    (
                                        vector<IImageBuffer *>  pylamid,
                                        IImageBuffer*           omc,
                                        IImageBuffer*           mcmv,
                                        int                     index,
                                        EGNParams<MSSConfig>&   rMssParams
                                    );

    enum MfllErr                    generateMsfParam
                                    (
                                        vector<IImageBuffer*>   base,
                                        vector<IImageBuffer*>   ref,
                                        vector<IImageBuffer*>   out,
                                        vector<IImageBuffer*>   wt_in,
                                        vector<IImageBuffer*>   wt_out,
                                        vector<IImageBuffer*>   wt_ds,
                                        IImageBuffer*           difference,
                                        IImageBuffer*           confmap,
                                        IImageBuffer*           dsFrame,
                                        EGNParams<MSFConfig>&   rMsfParams
                                    );

    enum MfllErr                    generateMsfP2Param
                                    (
                                        vector<IImageBuffer*>   base,
                                        vector<IImageBuffer*>   ref,
                                        vector<IImageBuffer*>   out,
                                        vector<IImageBuffer*>   wt_in,
                                        vector<IImageBuffer*>   wt_out,
                                        vector<IImageBuffer*>   wt_ds,
                                        IImageBuffer*           difference,
                                        IImageBuffer*           confmap,
                                        IImageBuffer*           dsFrame,
                                        EGNParams<MSFConfig>&   rMsfParams,
                                        MINT32                  stage
                                    );

    enum MfllErr                    generateNDDHint(FILE_DUMP_NAMING_HINT& hint, MINT32 frameIndex, MINT32 stage = 0);

    enum MfllErr                    initMfbTuningBuf(const vector<IImageBuffer *>& pylamid, IImageBuffer* dceso, MINT32 frameIndex);

    enum MfllErr                    updateMSFTuningBuf(const vector<IImageBuffer *>& pylamid, const unsigned int meanLuma);

    IMetadata*                      getMainAppMetaLocked() const;
    IMetadata*                      getMainHalMetaLocked() const;
    MetaSet_T                       getMainMetasetLocked() const;
    MetaSet_T                       getMetasetByIdx(MINT32 idx) const;

    /* IHalISP::setIsp wrapper */
    MINT32                          setIsp(
                                        MINT32          flowType,
                                        MetaSet_T&      control,
                                        TuningParam*    pTuningBuf,
                                        MetaSet_T*      pResult = nullptr,
                                        IImageBuffer*   pP2SrcImg = nullptr
                                    );


/* attributes */
private:
    /* thread-safe protector */
    mutable Mutex m_mutex;

private:
    int                             m_sensorId;
    enum MfllMode                   m_shotMode;
    enum NoiseReductionType         m_nrType;
    void*                           m_syncPrivateData;
    size_t                          m_syncPrivateDataSize;

    IMfllCore*                      m_pCore;

    volatile int                    m_encodeYuvCount;
    volatile unsigned int           m_blendCount;
    volatile bool                   m_bIsInited;

    bool                            m_bExifDumpped[STAGE_SIZE];

    IMetadata*                      m_pMainMetaApp;
    IMetadata*                      m_pMainMetaHal;

    sp<IMfllImageBuffer>            m_pMixDebugBuffer;

    /**
     *  MFB2.0 always use the "same" metadata to execute P2 operation which
     *  is requested from ALG team.
     */
    std::deque<MetaSet_T>           m_vMetaSet;
    std::deque<IMetadata*>          m_vMetaApp;
    std::deque<IMetadata*>          m_vMetaHal;
    std::deque<IImageBuffer*>       m_vImgLcsoRaw;


    std::unique_ptr< IHalISP, std::function<void(IHalISP*)> >
                                    m_pHalISP;

    std::unique_ptr< INormalStream, std::function<void(INormalStream*)> >
                                    m_pNormalStream;

    /* New MSF API */
    std::unique_ptr<
        IEgnStream<MSFConfig>,
        std::function<void(IEgnStream<MSFConfig>*)>
    >
                                    m_pMsfStream;
    /* New MSF API */
    std::unique_ptr<
        IEgnStream<MSSConfig>,
        std::function<void(IEgnStream<MSSConfig>*)>
    >
                                    m_pMssStream;


/* tuning buffers (ISP) */
private:
    struct MyTuningBuf
    {
        std::unique_ptr<char[]>     data;
        size_t                      size;
        volatile bool               isInited;
        //
        MyTuningBuf() : size(0), isInited(false) {}
        ~MyTuningBuf() = default;
    };

    /* tuning buffers */
    MyTuningBuf                     m_tuningBuf;
    MyTuningBuf                     m_tuningBufMsf[MFLL_MAX_FRAMES][MFLL_MAX_PYRAMID_SIZE];
    MyTuningBuf                     m_tuningBufMsfTbl[MFLL_MAX_FRAMES][MFLL_MAX_PYRAMID_SIZE];
    MyTuningBuf                     m_tuningBufMsfP2[MFLL_MAX_PYRAMID_SIZE];
    MyTuningBuf                     m_tuningBufMss[MFLL_MAX_FRAMES];
    FILE_DUMP_NAMING_HINT           m_nddHintMss;
    FILE_DUMP_NAMING_HINT           m_nddHintMsf;
    MfllMfb_Imp::P2Cookie           m_mssCookie[MFLL_MAX_FRAMES];
    MfllMfb_Imp::P2Cookie           m_msfCookie[MFLL_MAX_FRAMES];

    EGNParams<MSSConfig>            m_vMssConfig;
    EGNParams<MSFConfig>            m_vMsfConfig;

    /* tuning image buffers, for lock BFBLD usage */
    void*                           m_pBPCI;

};
};/* namespace mfll */
#endif//__MfllMfb_H__
