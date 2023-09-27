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
#ifndef DPE_CONFIG_H_
#define DPE_CONFIG_H_

#define PACKING
typedef unsigned int FIELD;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_cand_num : 8;
        FIELD c_dv_start_pos : 9;
        FIELD c_cen_ws_mode : 2;
        FIELD c_cen_color_thd : 4;
        FIELD c_cen_avg_ws_mode : 3;
        FIELD c_ad_weight : 3;
        FIELD rsv_29 : 3;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_00, *PREG_DVS_ME_00;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_cen_c00 : 8;
        FIELD c_lut_cen_c01 : 8;
        FIELD c_lut_cen_c02 : 8;
        FIELD c_lut_cen_c03 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_01, *PREG_DVS_ME_01;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_cen_c04 : 8;
        FIELD c_lut_cen_c05 : 8;
        FIELD c_lut_cen_c06 : 8;
        FIELD c_lut_cen_c07 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_02, *PREG_DVS_ME_02;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_cen_c08 : 8;
        FIELD c_lut_cen_c09 : 8;
        FIELD c_lut_ad_c00 : 8;
        FIELD c_lut_ad_c01 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_03, *PREG_DVS_ME_03;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_ad_c02 : 8;
        FIELD c_lut_ad_c03 : 8;
        FIELD c_lut_ad_c04 : 8;
        FIELD c_lut_ad_c05 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_04, *PREG_DVS_ME_04;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_ad_c06 : 8;
        FIELD c_lut_ad_c07 : 8;
        FIELD c_lut_ad_c08 : 8;
        FIELD c_lut_ad_c09 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_05, *PREG_DVS_ME_05;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_ad_c10 : 8;
        FIELD c_lut_conf_c00 : 7;
        FIELD c_lut_conf_c01 : 7;
        FIELD c_lut_conf_c02 : 7;
        FIELD rsv_29 : 3;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_06, *PREG_DVS_ME_06;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_conf_c03 : 7;
        FIELD c_lut_conf_c04 : 7;
        FIELD c_lut_conf_c05 : 7;
        FIELD c_lut_conf_c06 : 7;
        FIELD rsv_28 : 4;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_07, *PREG_DVS_ME_07;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_recursive_mode : 1;
        FIELD c_rpd_en : 1;
        FIELD c_rpd_thd : 6;
        FIELD c_rpd_chk_ratio : 3;
        FIELD rsv_11 : 21;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_08, *PREG_DVS_ME_08;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p1_c00 : 10;
        FIELD c_lut_p1_c01 : 10;
        FIELD c_lut_p1_c02 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_09, *PREG_DVS_ME_09;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p1_c03 : 10;
        FIELD c_lut_p1_c04 : 10;
        FIELD c_lut_p1_c05 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_10, *PREG_DVS_ME_10;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p1_c06 : 10;
        FIELD c_lut_p1_c07 : 10;
        FIELD c_lut_p1_c08 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_11, *PREG_DVS_ME_11;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p1_c09 : 10;
        FIELD c_lut_p1_c10 : 10;
        FIELD c_lut_p1_c11 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_12, *PREG_DVS_ME_12;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p1_c12 : 10;
        FIELD c_lut_p1_c13 : 10;
        FIELD c_lut_p1_c14 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_13, *PREG_DVS_ME_13;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p2_c00 : 10;
        FIELD c_lut_p2_c01 : 10;
        FIELD c_lut_p2_c02 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_14, *PREG_DVS_ME_14;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p2_c03 : 10;
        FIELD c_lut_p2_c04 : 10;
        FIELD c_lut_p2_c05 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_15, *PREG_DVS_ME_15;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p2_c06 : 10;
        FIELD c_lut_p2_c07 : 10;
        FIELD c_lut_p2_c08 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_16, *PREG_DVS_ME_16;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p2_c09 : 10;
        FIELD c_lut_p2_c10 : 10;
        FIELD c_lut_p2_c11 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_17, *PREG_DVS_ME_17;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p2_c12 : 10;
        FIELD c_lut_p2_c13 : 10;
        FIELD c_lut_p2_c14 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_18, *PREG_DVS_ME_18;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p3_c00 : 10;
        FIELD c_lut_p3_c01 : 10;
        FIELD c_lut_p3_c02 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_19, *PREG_DVS_ME_19;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p3_c03 : 10;
        FIELD c_lut_p3_c04 : 10;
        FIELD c_lut_p3_c05 : 10;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_20, *PREG_DVS_ME_20;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lut_p3_c06 : 10;
        FIELD c_lut_p3_c07 : 10;
        FIELD rsv_20 : 12;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_21, *PREG_DVS_ME_21;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wgt_dir_0 : 6;
        FIELD c_wgt_dir_1_3 : 5;
        FIELD c_wgt_dir_2 : 5;
        FIELD c_wgt_dir_4 : 6;
        FIELD rsv_22 : 10;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_22, *PREG_DVS_ME_22;

typedef PACKING union
{
    PACKING struct
    {
        FIELD rsv_0 : 6;
        FIELD c_fractional_cv : 2;
        FIELD c_subpixel_ip : 3;
        FIELD rsv_11 : 21;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_23, *PREG_DVS_ME_23;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_conf_dv_diff : 1;
        FIELD rsv_1 : 3;
        FIELD c_bypass_p3_thd : 3;
        FIELD c_p3_by_conf_rpd : 3;
        FIELD c_p3_conf_thd : 3;
        FIELD c_conf_cost_diff : 10;
        FIELD rsv_23 : 9;
    } Bits;
    unsigned int Raw;
} REG_DVS_ME_24, *PREG_DVS_ME_24;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_owc_upd_uq_en : 1;
        FIELD c_owc_uq_value : 3;
        FIELD c_owc_en : 1;
        FIELD c_owc_tlr_thr : 12;
        FIELD rsv_17 : 15;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_0, *PREG_DVS_OCC_PQ_0;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_ohf_upd_uq_en : 1;
        FIELD c_ohf_uq_value : 3;
        FIELD c_ohf_en : 1;
        FIELD c_ohf_tlr_thr : 12;
        FIELD rsv_17 : 15;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_1, *PREG_DVS_OCC_PQ_1;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_lrc_upd_uq_en : 1;
        FIELD c_lrc_thr_ng : 12;
        FIELD c_lrc_thr : 12;
        FIELD rsv_25 : 7;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_2, *PREG_DVS_OCC_PQ_2;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_sbf_h_size_mode : 2;
        FIELD c_sbf_v_size_mode : 1;
        FIELD c_sbf_thr : 8;
        FIELD rsv_11 : 21;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_3, *PREG_DVS_OCC_PQ_3;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_ups_dv_thr : 8;
        FIELD c_ups_bld_en : 1;
        FIELD c_ups_bld_step : 1;
        FIELD c_ups_weight_mode : 1;
        FIELD c_occ_dv_min_uq_thr : 3;
        FIELD c_occ_dv_rmv_rp_en : 1;
        FIELD c_occ_dv_msb_sft : 2;
        FIELD c_occ_dv_lsb_sft : 2;
        FIELD c_occ_dv_offset : 12;
        FIELD c_occ_flag_debug : 1;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_4, *PREG_DVS_OCC_PQ_4;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_fx_baseline : 29;
        FIELD c_depth_format : 2;
        FIELD rsv_31 : 1;
    } Bits;
    unsigned int Raw;
} REG_DVS_OCC_PQ_5, *PREG_DVS_OCC_PQ_5;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_asf_thd_y : 8;
        FIELD c_asf_thd_cbcr : 8;
        FIELD c_asf_arm_ud : 4;
        FIELD c_asf_arm_lr : 5;
        FIELD rsv_25 : 7;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_00, *PREG_DVP_CORE_00;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_rmv_mean_lut_0 : 8;
        FIELD c_rmv_mean_lut_1 : 8;
        FIELD c_rmv_mean_lut_2 : 8;
        FIELD c_rmv_mean_lut_3 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_01, *PREG_DVP_CORE_01;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_rmv_mean_lut_4 : 8;
        FIELD c_rmv_mean_lut_5 : 8;
        FIELD c_rmv_mean_lut_6 : 8;
        FIELD c_rmv_mean_lut_7 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_02, *PREG_DVP_CORE_02;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_rmv_dsty_lut_0 : 5;
        FIELD rsv_5 : 3;
        FIELD c_rmv_dsty_lut_1 : 5;
        FIELD rsv_13 : 3;
        FIELD c_rmv_dsty_lut_2 : 5;
        FIELD rsv_21 : 3;
        FIELD c_rmv_dsty_lut_3 : 5;
        FIELD rsv_29 : 3;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_03, *PREG_DVP_CORE_03;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_rmv_dsty_lut_4 : 5;
        FIELD rsv_5 : 3;
        FIELD c_rmv_dsty_lut_5 : 5;
        FIELD rsv_13 : 3;
        FIELD c_rmv_dsty_lut_6 : 5;
        FIELD rsv_21 : 3;
        FIELD c_rmv_dsty_lut_7 : 5;
        FIELD rsv_29 : 3;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_04, *PREG_DVP_CORE_04;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_hf_thd_r1 : 6;
        FIELD rsv_6 : 2;
        FIELD c_hf_thd_r2 : 6;
        FIELD rsv_14 : 2;
        FIELD c_hf_thd_r3 : 6;
        FIELD rsv_22 : 2;
        FIELD c_hf_thd_r4 : 6;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_05, *PREG_DVP_CORE_05;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_hf_thd_r5 : 6;
        FIELD rsv_6 : 2;
        FIELD c_hf_thd_r6 : 6;
        FIELD rsv_14 : 2;
        FIELD c_hf_thd_r7 : 6;
        FIELD rsv_22 : 2;
        FIELD c_hf_thd_r8 : 6;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_06, *PREG_DVP_CORE_06;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_hf_thin_en : 1;
        FIELD rsv_1 : 3;
        FIELD c_hf_thd_thin : 2;
        FIELD rsv_6 : 10;
        FIELD c_hf_thd_r9 : 6;
        FIELD rsv_22 : 2;
        FIELD c_hf_thd_r10 : 6;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_07, *PREG_DVP_CORE_07;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_0 : 8;
        FIELD c_wmf_wgt_lut_1 : 8;
        FIELD c_wmf_wgt_lut_2 : 8;
        FIELD c_wmf_wgt_lut_3 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_08, *PREG_DVP_CORE_08;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_4 : 8;
        FIELD c_wmf_wgt_lut_5 : 8;
        FIELD c_wmf_wgt_lut_6 : 8;
        FIELD c_wmf_wgt_lut_7 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_09, *PREG_DVP_CORE_09;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_8 : 8;
        FIELD c_wmf_wgt_lut_9 : 8;
        FIELD c_wmf_wgt_lut_10 : 8;
        FIELD c_wmf_wgt_lut_11 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_10, *PREG_DVP_CORE_10;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_12 : 8;
        FIELD c_wmf_wgt_lut_13 : 8;
        FIELD c_wmf_wgt_lut_14 : 8;
        FIELD c_wmf_wgt_lut_15 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_11, *PREG_DVP_CORE_11;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_16 : 8;
        FIELD c_wmf_wgt_lut_17 : 8;
        FIELD c_wmf_wgt_lut_18 : 8;
        FIELD c_wmf_wgt_lut_19 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_12, *PREG_DVP_CORE_12;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_20 : 8;
        FIELD c_wmf_wgt_lut_21 : 8;
        FIELD c_wmf_wgt_lut_22 : 8;
        FIELD c_wmf_wgt_lut_23 : 8;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_13, *PREG_DVP_CORE_13;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_wmf_wgt_lut_24 : 8;
        FIELD rsv_8 : 24;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_14, *PREG_DVP_CORE_14;

typedef PACKING union
{
    PACKING struct
    {
        FIELD c_hf_mode_r1 : 3;
        FIELD c_hf_mode_r2 : 3;
        FIELD c_hf_mode_r3 : 3;
        FIELD c_hf_mode_r4 : 3;
        FIELD c_hf_mode_r5 : 3;
        FIELD c_hf_mode_r6 : 3;
        FIELD c_hf_mode_r7 : 3;
        FIELD c_hf_mode_r8 : 3;
        FIELD c_hf_mode_r9 : 3;
        FIELD c_hf_mode_r10 : 3;
        FIELD rsv_30 : 2;
    } Bits;
    unsigned int Raw;
} REG_DVP_CORE_15, *PREG_DVP_CORE_15;

typedef struct
{
    REG_DVS_ME_00    DVS_ME_00;
    REG_DVS_ME_01    DVS_ME_01;
    REG_DVS_ME_02    DVS_ME_02;
    REG_DVS_ME_03    DVS_ME_03;
    REG_DVS_ME_04    DVS_ME_04;
    REG_DVS_ME_05    DVS_ME_05;
    REG_DVS_ME_06    DVS_ME_06;
    REG_DVS_ME_07    DVS_ME_07;
    REG_DVS_ME_08    DVS_ME_08;
    REG_DVS_ME_09    DVS_ME_09;
    REG_DVS_ME_10    DVS_ME_10;
    REG_DVS_ME_11    DVS_ME_11;
    REG_DVS_ME_12    DVS_ME_12;
    REG_DVS_ME_13    DVS_ME_13;
    REG_DVS_ME_14    DVS_ME_14;
    REG_DVS_ME_15    DVS_ME_15;
    REG_DVS_ME_16    DVS_ME_16;
    REG_DVS_ME_17    DVS_ME_17;
    REG_DVS_ME_18    DVS_ME_18;
    REG_DVS_ME_19    DVS_ME_19;
    REG_DVS_ME_20    DVS_ME_20;
    REG_DVS_ME_21    DVS_ME_21;
    REG_DVS_ME_22    DVS_ME_22;
    REG_DVS_ME_23    DVS_ME_23;
    REG_DVS_ME_24    DVS_ME_24;
} REG_DVS_ME_CFG, *PDVS_ME_CFG;

typedef struct
{
    REG_DVS_OCC_PQ_0    DVS_OCC_PQ_0;
    REG_DVS_OCC_PQ_1    DVS_OCC_PQ_1;
    REG_DVS_OCC_PQ_2    DVS_OCC_PQ_2;
    REG_DVS_OCC_PQ_3    DVS_OCC_PQ_3;
    REG_DVS_OCC_PQ_4    DVS_OCC_PQ_4;
    REG_DVS_OCC_PQ_5    DVS_OCC_PQ_5;
} REG_DVS_OCC_PQ_CFG, *PDVS_OCC_PQ_CFG;

typedef struct
{
    REG_DVP_CORE_00    DVP_CORE_00;
    REG_DVP_CORE_01    DVP_CORE_01;
    REG_DVP_CORE_02    DVP_CORE_02;
    REG_DVP_CORE_03    DVP_CORE_03;
    REG_DVP_CORE_04    DVP_CORE_04;
    REG_DVP_CORE_05    DVP_CORE_05;
    REG_DVP_CORE_06    DVP_CORE_06;
    REG_DVP_CORE_07    DVP_CORE_07;
    REG_DVP_CORE_08    DVP_CORE_08;
    REG_DVP_CORE_09    DVP_CORE_09;
    REG_DVP_CORE_10    DVP_CORE_10;
    REG_DVP_CORE_11    DVP_CORE_11;
    REG_DVP_CORE_12    DVP_CORE_12;
    REG_DVP_CORE_13    DVP_CORE_13;
    REG_DVP_CORE_14    DVP_CORE_14;
    REG_DVP_CORE_15    DVP_CORE_15;
} REG_DVP_CORE_CFG, *PDVP_CORE_CFG;

#endif