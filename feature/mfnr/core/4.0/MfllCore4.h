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
#ifndef __MFLLCORE4_H__
#define __MFLLCORE4_H__

#include "MfllCore.h"
#include <mtkcam3/feature/mfnr/IMfllImageBuffer.h>

// AOSP
#include <utils/RefBase.h>

// STL
#include <memory>
#include <vector>
#include <mutex>
#include <future>

namespace mfll {

class MfllCore4 : public MfllCore
{
public:
    MfllCore4(void);
    virtual ~MfllCore4(void) = default;


//
// Override
//
public:
    virtual sp<IMfllImageBuffer> retrieveBuffer(const enum MfllBuffer &s, int index = 0) override;
    virtual enum MfllErr        releaseBuffer(const enum MfllBuffer &s, int index = 0) override;

    virtual enum MfllErr        do_Init(const MfllConfig_t& cfg)            override;
    virtual enum MfllErr        do_AllocMssBuffer(void* void_index)         override;
    virtual enum MfllErr        do_AllocMemcWorking(void* void_index)       override;
    virtual enum MfllErr        do_AllocYuvGolden(JOB_VOID)                 override;
    virtual enum MfllErr        do_MotionEstimation(void* void_index)       override;
    virtual enum MfllErr        do_MotionCompensation(void* void_index)     override;
    virtual enum MfllErr        do_EncodeMss(void *void_index)              override;
    virtual enum MfllErr        do_Msf(void *void_index)                    override;
    virtual unsigned int        getVersion()                                override;
    virtual std::string         getVersionString()                          override;
    virtual vector<int>         getMemcFrameLvConfs(void)                   override;


//
// New implementations
//
protected:

    // To create a difference image for MSF. The difference image
    // is an NV21 image.
    //  @param width            Width in pixel of source image (full size).
    //  @param height           Height in pixel of source image (full size).
    virtual android::sp<IMfllImageBuffer>
                                createDifferenceImage(
                                    size_t width,
                                    size_t height
                                );


    // To create a confidence map image for MEMC v1.1 (and upon). The confidence
    // map is an one plane 8 bits image.
    //  @param width            Width in pixel of source image (full size).
    //  @param height           Height in pixel of source image (full size).
    virtual android::sp<IMfllImageBuffer>
                                createConfidenceMap(
                                    size_t width,
                                    size_t height
                                );

    // To create a motion compensation motion vector for MEMC v1.3 (and upon). The motion
    // vector is an one plane 8 bits image.
    //  @param width            Width in pixel of source image (full size).
    //  @param height           Height in pixel of source image (full size).
    virtual android::sp<IMfllImageBuffer>
                                createMotionCompensationMV(
                                    size_t width,
                                    size_t height
                                );

    // To create a DCESO buffer for Golden/ Mix.  The format and size are queried from IspHal
    virtual android::sp<IMfllImageBuffer>
                                createDcesoBuffer();


    // To create a Mix debug buffer for Mix.
    virtual android::sp<IMfllImageBuffer>
                                createMixDebugBuffer(sp<IMfllImageBuffer> ref);

//
// Attributes
//
protected:
    // Confidence map amount equals to blend frame number
    vector<int>     m_aryMemcFrameLevelConf;
    ImageBufferPack m_imgDifferenceImage[MFLL_MAX_FRAMES];
    ImageBufferPack m_imgConfidenceMaps[MFLL_MAX_FRAMES];
    ImageBufferPack m_imgMotionCompensationMvs[MFLL_MAX_FRAMES];
    //ImageBufferPack m_imgDCESO;
    ImageBufferPack m_imgMixDebug;


}; /* class MfllCore4 */
}; /* namespace mfll */
#endif /* __MFLLCORE4_H__ */
