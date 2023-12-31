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
#ifndef __MFLLMFB_ALGO_H__
#define __MFLLMFB_ALGO_H__

#ifndef OFFLINE_TEST
#ifndef LOG_TAG
#define LOG_TAG "MfllCore/Mfb"
#include <mtkcam3/feature/mfnr/MfllLog.h>
#endif

// platform dependent headers
#include <dip_reg.h> // dip_x_reg_t

// AOSP
#include <cutils/compiler.h>
#endif //OFFLINE_TEST

// STL
#include <math.h>

// define this macro to 0 to disable refine function for avoiding compile errors
#define MFLL_COMPILE_POST_NR_REFINE     0

#define RF_CLIP(a,b,c)   (( (a) < (b) )?  (b) : (( (a) > (c) )? (c):(a) ))

namespace mfll
{

inline int reg_div_gain(
    int val,
    float gain,
    int min,
    int max
    )
{
    int ret = val >= 0 ? (int)((float(val) / gain) + 0.5) : (int)((float(val) / gain) - 0.5);
    ret = RF_CLIP(ret, min, max);
    return ret;
}

inline int reg_mul_gain(
    int val,
    float gain,
    int min,
    int max
    )
{
    int ret = val >= 0 ? (int)((float(val)*gain) + 0.5) : (int)((float(val)*gain) - 0.5);
    ret = RF_CLIP(ret, min, max);
    return ret;
}

inline int reg_gain_max(
    int val,
    int min,
    int max,
    float gain
    )
{
    float fDelta = float(max - val) / gain;
    int ret = (fDelta >= 0) ? max - (int)(fDelta + 0.5) : max - (int)(fDelta - 0.5);
    ret = RF_CLIP(ret, min, max);
    return ret;
}

inline int mix_m1_refine(int m1,int m0, int curFrames, int expFrames)
{
    int m1_out = m0+9; //If we set MIX_SLOPE_SEL=1, the maximum slop is 31.875. => minimum step is 9
    if (expFrames >= 2*256) {
        float fDelta = float(m1-m0)*float(curFrames-256)/float(expFrames-256);
        int i4Delta;
        if (fDelta>=0) { i4Delta=(int)(fDelta+0.5); }
        else { i4Delta=(int)(fDelta-0.5); }
        if (i4Delta<=0) i4Delta=0;
        m1_out = m0+i4Delta;
        if (m1_out<= m0 + 9) m1_out= m0 + 9;//If we set MIX_SLOPE_SEL=1, the maximum slop is 31.875. => minimum step is 9
    }

    return m1_out;
}

inline int mix_cal_slope(int m1, int m0, int b1, int b0)
{
    return (int)((float)(b1 - b0) * 8 / (m1 - m0) + 0.5);
}

// PostNR refine function implementation.
//  @param [in/out] regs            Registers structure, see drv/isp_reg.h
//  @param [in]     frm_lvl_confs   Array of frame level confidence from ME.
//                                  The number of element is real_cnt.
//  @param [in]     real_cnt        Real blending count. If takes 6 frames, assumes
//                                  all frames are ok for blending, this value will
//                                  be 5 (5 times of blending).
//  @param [in]     expected_cnt    Expected blending count. If takes 6 frames,
//                                  this value will be 5 (5 times to blend).
//  @note                           Notice that, arguments real_cnt and expected_cnt
//                                  will be (CaptureFrameNum - 1).
inline void bss_refine_postnr_regs(
        void*   regs,
        int     *frm_lvl_confs,
        int     real_cnt,
        int     expected_cnt,
        int     intensity = 16
        )
{
#if MFLL_COMPILE_POST_NR_REFINE
    dip_x_reg_t* pRegs = static_cast<dip_x_reg_t*>(regs);

    mfllLogD("%s: blend frames count, real=%d, expected=%d",
            __FUNCTION__, real_cnt, expected_cnt);

    if (CC_UNLIKELY( real_cnt < 0 )) {
        mfllLogW("%s: read blend count < 0, ignore refine", __FUNCTION__);
        return;
    }

    if (CC_UNLIKELY( expected_cnt < 0 )) {
        mfllLogW("%s: expected blending count is < 0, ignore refine", __FUNCTION__);
        return;
    }

    mfllLogD("%s: refine postNR", __FUNCTION__);

    // increase real_cnt and expected_cnt as the domain of CaptureFrameNum
    //real_cnt++;
    //expected_cnt++;

    //convert to 256 based frame conunt. (256 = 1 frame)
    int x256expected_cnt = 256 * (expected_cnt+1);
    int x256real_cnt = 256;
    for (int i = 0; i < real_cnt; i++)
        x256real_cnt += frm_lvl_confs[i];


    // ------------------------------------------------------------------------
    // implement refine function here
    // ------------------------------------------------------------------------

    float fSTDGain1 = (float)pow(double(x256expected_cnt)/double(x256real_cnt), 0.25+double(intensity)/64.0);
    float fSTDGain2 = (float)pow(double(x256expected_cnt)/double(x256real_cnt), double(intensity)/48.0);

    int mix_m1,mix_m0,mix_dt,mix_b1,mix_b0;

//    float f_mix_dt;

    mfllLogD3("%s: original: YNR_Y_HF_ACT_Y0(%d), YNR_Y_HF_ACT_Y1(%d), YNR_Y_HF_ACT_Y2(%d), YNR_Y_HF_ACT_Y3(%d), YNR_Y_HF_ACT_Y4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y0,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y1,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y2,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y3,
        pRegs->YNR_D1A_YNR_HF_ACT2.Bits.YNR_Y_HF_ACT_Y4);
    mfllLogD3("%s: original: YNR_Y_HF_LUMA_Y0(%d), YNR_Y_HF_LUMA_Y1(%d), YNR_Y_HF_LUMA_Y2(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1,
        pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2);

    mfllLogD3("%s: original: YNR_Y_HF_LUMA_Y0(%d), YNR_Y_HF_LUMA_Y1(%d), YNR_Y_HF_LUMA_Y2(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1,
        pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2);

    mfllLogD3("%s: original: YNR_Y_L0_H_RNG1(%d), YNR_Y_L0_H_RNG2(%d), YNR_Y_L0_H_RNG3(%d), YNR_Y_L0_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG4);

    mfllLogD3("%s: original: YNR_Y_L1_H_RNG1(%d), YNR_Y_L1_H_RNG2(%d), YNR_Y_L1_H_RNG3(%d), YNR_Y_L1_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG4);

    mfllLogD3("%s: original: YNR_Y_L2_H_RNG1(%d), YNR_Y_L2_H_RNG2(%d), YNR_Y_L2_H_RNG3(%d), YNR_Y_L2_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG4);

    mfllLogD3("%s: original: YNR_Y_L3_H_RNG1(%d), YNR_Y_L3_H_RNG2(%d), YNR_Y_L3_H_RNG3(%d), YNR_Y_L3_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG4);

    mfllLogD3("%s: original: YNR_Y_L0_V_RNG1(%d), YNR_Y_L0_V_RNG2(%d), YNR_Y_L0_V_RNG3(%d), YNR_Y_L0_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG4);

    mfllLogD3("%s: original: YNR_Y_L1_V_RNG1(%d), YNR_Y_L1_V_RNG2(%d), YNR_Y_L1_V_RNG3(%d), YNR_Y_L1_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG4);

    mfllLogD3("%s: original: YNR_Y_L2_V_RNG1(%d), YNR_Y_L2_V_RNG2(%d), YNR_Y_L2_V_RNG3(%d), YNR_Y_L2_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG4);

    mfllLogD3("%s: original: YNR_Y_L3_V_RNG1(%d), YNR_Y_L3_V_RNG2(%d), YNR_Y_L3_V_RNG3(%d), YNR_Y_L3_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG4);

    mfllLogD3("%s: original: YNR_Y_SLOPE_H_TH(%d), YNR_Y_SLOPE_V_TH(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_H_TH,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_V_TH);

    mfllLogD3("%s: original: YNR_C_V_RNG1(%d), YNR_C_V_RNG2(%d), YNR_C_V_RNG3(%d), YNR_C_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG1,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG2,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG3,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG4);

    mfllLogD3("%s: original: YNR_C_H_RNG1(%d), YNR_C_H_RNG2(%d), YNR_C_H_RNG3(%d), YNR_C_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG1,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG2,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG3,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG4);

    mfllLogD3("%s: original: YNR_CEN_GAIN_LO_TH(%d), YNR_CEN_GAIN_HI_TH(%d), YNR_CEN_GAIN_LO_TH_LPF(%d), YNR_CEN_GAIN_HI_TH_LPF(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_LO_TH,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_HI_TH,
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_LO_TH_LPF,
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_HI_TH_LPF);


    if (x256real_cnt != x256expected_cnt) {

        //BLD
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y0 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y0, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y1 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y1, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y2 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y2, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y3 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y3, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_ACT2.Bits.YNR_Y_HF_ACT_Y4 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_ACT2.Bits.YNR_Y_HF_ACT_Y4, fSTDGain2, 0, 64);

        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1, fSTDGain2, 0, 64);
        pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2 = reg_div_gain(pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2, fSTDGain2, 0, 64);

        //PTY
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG4, fSTDGain1, 0, 255);

        //SLOPE_TH
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_H_TH = reg_mul_gain(pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_H_TH, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_V_TH = reg_mul_gain(pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_V_TH, fSTDGain1, 0, 255);

        //PTC
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG4, fSTDGain1, 0, 255);

        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG1 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG1, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG2 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG2, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG3 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG3, fSTDGain1, 0, 255);
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG4 = reg_mul_gain(pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG4, fSTDGain1, 0, 255);

        //CEN_GAIN
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_LO_TH = reg_gain_max(pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_LO_TH, 0 ,16, fSTDGain2);
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_HI_TH = reg_gain_max(pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_HI_TH, 0, 16, fSTDGain2);
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_LO_TH_LPF = reg_gain_max(pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_LO_TH_LPF, 0 ,16, fSTDGain2);
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_HI_TH_LPF = reg_gain_max(pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_HI_TH_LPF, 0 ,16, fSTDGain2);

        //MIXER
        mix_m1 = mix_m1_refine(pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M1, pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M0, x256real_cnt, x256expected_cnt);
    }
    else {
        mix_m1 = pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M1;
    }

    mfllLogD3("%s: refined: YNR_Y_HF_ACT_Y0(%d), YNR_Y_HF_ACT_Y1(%d), YNR_Y_HF_ACT_Y2(%d), YNR_Y_HF_ACT_Y3(%d), YNR_Y_HF_ACT_Y4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y0,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y1,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y2,
        pRegs->YNR_D1A_YNR_HF_ACT1.Bits.YNR_Y_HF_ACT_Y3,
        pRegs->YNR_D1A_YNR_HF_ACT2.Bits.YNR_Y_HF_ACT_Y4);
    mfllLogD3("%s: refined: YNR_Y_HF_LUMA_Y0(%d), YNR_Y_HF_LUMA_Y1(%d), YNR_Y_HF_LUMA_Y2(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1,
        pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2);

    mfllLogD3("%s: refined: YNR_Y_HF_LUMA_Y0(%d), YNR_Y_HF_LUMA_Y1(%d), YNR_Y_HF_LUMA_Y2(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y0,
        pRegs->YNR_D1A_YNR_HF_LUMA0.Bits.YNR_Y_HF_LUMA_Y1,
        pRegs->YNR_D1A_YNR_HF_LUMA1.Bits.YNR_Y_HF_LUMA_Y2);

    mfllLogD3("%s: refined: YNR_Y_L0_H_RNG1(%d), YNR_Y_L0_H_RNG2(%d), YNR_Y_L0_H_RNG3(%d), YNR_Y_L0_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY0H.Bits.YNR_Y_L0_H_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L1_H_RNG1(%d), YNR_Y_L1_H_RNG2(%d), YNR_Y_L1_H_RNG3(%d), YNR_Y_L1_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY1H.Bits.YNR_Y_L1_H_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L2_H_RNG1(%d), YNR_Y_L2_H_RNG2(%d), YNR_Y_L2_H_RNG3(%d), YNR_Y_L2_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY2H.Bits.YNR_Y_L2_H_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L3_H_RNG1(%d), YNR_Y_L3_H_RNG2(%d), YNR_Y_L3_H_RNG3(%d), YNR_Y_L3_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG1,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG2,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG3,
        pRegs->YNR_D1A_YNR_PTY3H.Bits.YNR_Y_L3_H_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L0_V_RNG1(%d), YNR_Y_L0_V_RNG2(%d), YNR_Y_L0_V_RNG3(%d), YNR_Y_L0_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY0V.Bits.YNR_Y_L0_V_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L1_V_RNG1(%d), YNR_Y_L1_V_RNG2(%d), YNR_Y_L1_V_RNG3(%d), YNR_Y_L1_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY1V.Bits.YNR_Y_L1_V_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L2_V_RNG1(%d), YNR_Y_L2_V_RNG2(%d), YNR_Y_L2_V_RNG3(%d), YNR_Y_L2_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY2V.Bits.YNR_Y_L2_V_RNG4);

    mfllLogD3("%s: refined: YNR_Y_L3_V_RNG1(%d), YNR_Y_L3_V_RNG2(%d), YNR_Y_L3_V_RNG3(%d), YNR_Y_L3_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG1,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG2,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG3,
        pRegs->YNR_D1A_YNR_PTY3V.Bits.YNR_Y_L3_V_RNG4);

    mfllLogD3("%s: refined: YNR_Y_SLOPE_H_TH(%d), YNR_Y_SLOPE_V_TH(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_H_TH,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_Y_SLOPE_V_TH);

    mfllLogD3("%s: refined: YNR_C_V_RNG1(%d), YNR_C_V_RNG2(%d), YNR_C_V_RNG3(%d), YNR_C_V_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG1,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG2,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG3,
        pRegs->YNR_D1A_YNR_PTCV.Bits.YNR_C_V_RNG4);

    mfllLogD3("%s: refined: YNR_C_H_RNG1(%d), YNR_C_H_RNG2(%d), YNR_C_H_RNG3(%d), YNR_C_H_RNG4(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG1,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG2,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG3,
        pRegs->YNR_D1A_YNR_PTCH.Bits.YNR_C_H_RNG4);

    mfllLogD3("%s: refined: YNR_CEN_GAIN_LO_TH(%d), YNR_CEN_GAIN_HI_TH(%d), YNR_CEN_GAIN_LO_TH_LPF(%d), YNR_CEN_GAIN_HI_TH_LPF(%d)", __FUNCTION__,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_LO_TH,
        pRegs->YNR_D1A_YNR_YAD1.Bits.YNR_CEN_GAIN_HI_TH,
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_LO_TH_LPF,
        pRegs->YNR_D1A_YNR_YLAD.Bits.YNR_CEN_GAIN_HI_TH_LPF);


    mix_m0 = pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M0;// do not change M0
    mix_b0 = pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_B0;// do not change M0
    mix_b1 = pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_B1;// do not change M0
                                                  //// final m0 modification
    //if (mix_m1<=(mix_m0+4)) mix_m1=(mix_m0+4);

    //f_mix_dt = 256.0/float(mix_m1-mix_m0);
    //if (f_mix_dt>=0) { mix_dt=(int)(f_mix_dt+0.5); }
    //else { mix_dt=(int)(f_mix_dt-0.5); }

    //mix_m1 = mix_m0 + 256/mix_dt;
    //mix_m1 = RF_CLIP(mix_m1,(mix_m0+4),(real_cnt*32));

    mix_dt = mix_cal_slope(mix_m1, mix_m0, mix_b1, mix_b0);

    mfllLogD3("%s: original: MIX_SLOPE_SEL(%d), MIX_M1(%d), MIX_M0(%d), MIX_DT(%d)", __FUNCTION__,
        pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_SLOPE_SEL,
        pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M1,
        pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M0,
        pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_DT);

    pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_SLOPE_SEL = 1;
    pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M1 = mix_m1;
    pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M0 = mix_m0;
    pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_DT = mix_dt;

    mfllLogD3("%s: refined: MIX_SLOPE_SEL(%d), MIX_M1(%d), MIX_M0(%d), MIX_DT(%d)", __FUNCTION__,
        pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_SLOPE_SEL,
        pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M1,
        pRegs->MIX_D3A_MIX_CTRL1.Bits.MIX_M0,
        pRegs->MIX_D3A_MIX_CTRL0.Bits.MIX_DT);

#else
    mfllLogD("%s: disabled post NR refine func", __FUNCTION__);
#endif
}

#ifndef OFFLINE_TEST

// Software workaround for MIX3, but performance trade off.
//  @param a_pbyImg             Virtual address of the image.
//  @param a_i4W                Width of the image.
//  @param a_i4H                Height of the image.
//  @return                     True for performed OK, false not.
bool fix_mix_yuy2(
        unsigned char*  a_pbyImg    __attribute__((unused)),
        int             a_i4W       __attribute__((unused)),
        int             a_i4H       __attribute__((unused))
)
{
    mfllLogD("%s: no function", __FUNCTION__);
    return false;
}
#endif// OFFLINE_TEST



// MFB refine function implementation.
//  @param [in/out] regs            Registers structure, see  drv/include/mfb/mt6779/mfb_reg.h
//  @param [in]     frm_lvl_conf    Frame level confidence output by SW ME.
//  @param [in]     proc_idx        The process index of MFB. (0: the first round MFB)
inline void me_refine_mfb_regs(
    void*   regs,
    int     frm_lvl_conf,
    int     proc_idx
)
{
#if 0
    static int ori_mx_wt = 32;
    mfb_reg_t* pRegs = static_cast<mfb_reg_t*>(regs);

    if (proc_idx == 0)
        ori_mx_wt = pRegs->MFB_MBD_CON0.Bits.MFB_BLD_MBD_MAX_WT;

    pRegs->MFB_MBD_CON0.Bits.MFB_BLD_MBD_MAX_WT = (ori_mx_wt*frm_lvl_conf+128) / 256;
#endif
}

bool calc_msf_idi_differnce_map(
    unsigned char*  pbyImgY_in_base,
    unsigned char*  pbyImgC_in_base,
    unsigned char*  pbyImgY_in_ref,
    unsigned char*  pbyImgC_in_ref,
    unsigned char*  pbyImgY_out,
    unsigned char*  pbyImgC_out,
    int             width,
    int             height,
    int             in_stride,
    int             in_bit,
    int             out_stride,
    int             out_bit,
    unsigned int&   mean_luma
)
{
    mfllLogD3("%s: version: 20190925", __FUNCTION__);
    mfllLogD3("%s: base: pbyImgY(%p), pbyImgC(%p), effW(%d), effH(%d), stride(%d), bit(%d)", __FUNCTION__, pbyImgY_in_base, pbyImgC_in_base, width, height, in_stride, in_bit);
    mfllLogD3("%s: ref:  pbyImgY(%p), pbyImgC(%p), effW(%d), effH(%d), stride(%d), bit(%d)", __FUNCTION__, pbyImgY_in_ref, pbyImgC_in_ref, width, height, in_stride, in_bit);
    mfllLogD3("%s: out:  pbyImgY(%p), pbyImgC(%p), effW(%d), effH(%d), stride(%d), bit(%d)", __FUNCTION__, pbyImgY_out, pbyImgC_out, width, height, out_stride, out_bit);
    if (in_bit != 16 || out_bit != 8) {
        mfllLogE("%s: not support for in_bit(%d), out_bit(%d)", __FUNCTION__, in_bit, out_bit);
        return false;
    }
    if (sizeof(short) != 2) {
        mfllLogE("%s: not support for short size(%zu) != 2", __FUNCTION__, sizeof(short));
        return false;
    }
    if (in_stride != 2 * out_stride) {
        mfllLogE("%s: not support for in_stride(%d), out_stride(%d)", __FUNCTION__, in_stride, out_stride);
        return false;
    }

    int i, j;
    int valueBase[3], valueRef[3], valueDiff[3];
    short *pixelBaseY = (short *)pbyImgY_in_base;
    short *pixelBaseC = (short *)pbyImgC_in_base;
    short *pixelRefY = (short *)pbyImgY_in_ref;
    short *pixelRefC = (short *)pbyImgC_in_ref;

    // Y Plane
    mfllLogD3("%s: process Y plane", __FUNCTION__);
    for (j = 0; j < height; j++)
    {
        for (i = 0; i < out_stride; i++)
        {
            //cout << "Y: (" << j << "," << i << ")" << endl;

            // Base
            valueBase[0] = (pixelBaseY[j*out_stride + i] >> 4) & 0xFFF;
            //cout << "Base:    " << valueBase[0] << endl;

            // Reference
            valueRef[0] = (pixelRefY[j*out_stride + i] >> 4) & 0xFFF;
            //cout << "Ref: " << valueRef[0] << endl;

            // Difference
            int valueTemp = valueBase[0] - valueRef[0];
            valueDiff[0] = round(valueTemp / 32.0);
            if (valueTemp == -48)
                valueDiff[0] = -2;
            else if ((valueTemp == -16) || (valueTemp == 16))
                valueDiff[0] = 0;
            //cout << "Diff:    " << valueDiff[0] << endl;

            pbyImgY_out[j*out_stride + i] = valueDiff[0];
            //cout << "Out: " << (int)(pbyImgY_out[j*out_stride + i]) << endl;
        }
    }
    // UV Plane =  1/2 Y Plane (2 * 1/4)
    mfllLogD3("%s: process UV plane", __FUNCTION__);
    int U_offset = height * width;
    for (j = 0; j < (height / 2); j++)
    {
        for (i = 0; i < (out_stride / 2); i++)
        {
            //for (int i = 0; i<(out_stride / 2)*height; i++)
            //cout << "C: (" << i << "," << j << ")" << endl;

            // Base
            valueBase[1] = ((pixelBaseC[j * (out_stride / 2) * 2 + i * 2 + 0] >> 4) & 0xFFF) - 2048;
            valueBase[2] = ((pixelBaseC[j * (out_stride / 2) * 2 + i * 2 + 1] >> 4) & 0xFFF) - 2048;
            //cout << "Base U:  " << valueBase[1] << endl;
            //cout << "Base V:  " << valueBase[2] << endl;

            // Reference
            valueRef[1] = ((pixelRefC[j * (out_stride / 2) * 2 + i * 2 + 0] >> 4) & 0xFFF) - 2048;
            valueRef[2] = ((pixelRefC[j * (out_stride / 2) * 2 + i * 2 + 1] >> 4) & 0xFFF) - 2048;
            //cout << "Ref U:   " << valueRef[1] << endl;
            //cout << "Ref V:   " << valueRef[2] << endl;

            // Difference
            int valueTempU = valueBase[1] - valueRef[1];
            valueDiff[1] = round(valueTempU / 32.0);
            if (valueTempU == -48)
                valueDiff[1] = -2;
            else if ((valueTempU == -16) || (valueTempU == 16))
                valueDiff[1] = 0;
            //cout << "Diff U:  " << valueDiff[1] << endl;
            int valueTempV = valueBase[2] - valueRef[2];
            valueDiff[2] = round(valueTempV / 32.0);
            if (valueTempV == -48)
                valueDiff[2] = -2;
            else if ((valueTempV == -16) || (valueTempV == 16))
                valueDiff[2] = 0;
            //cout << "Diff V:  " << valueDiff[2] << endl;

            pbyImgC_out[j * (out_stride / 2) * 2 + i * 2 + 0] = valueDiff[1];
            pbyImgC_out[j * (out_stride / 2) * 2 + i * 2 + 1] = valueDiff[2];
        }
    }

    // For mean luminance calculation
    mean_luma = 0;
    for (j = 0; j < height; j++)
        for (i = 0; i < width; i++)
            mean_luma += ((pixelBaseY[j*out_stride + i] >> 4) & 0xFFF);
    mean_luma /= (height * width);
    // 12-Bit -> 8-Bit
    mean_luma = (mean_luma >> 4);

    return true;
}


};/* namespace mfll */
#endif//__MFLLMFB_ALGO_H__
