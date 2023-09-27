/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef _RSC_HANDLER_H_
#define _RSC_HANDLER_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "red.h"

#define MTK_ISP_CTX_RSC_TUNING_DATA_NUM      23
//#define RSC_BASE_HW                          0x1b003000

static uint32_t  rsc_reg_base;

/* RSC unmapped base address macro for GCE to access */
#define RSC_RST_HW                    (rsc_reg_base)
#define RSC_START_HW                  (rsc_reg_base + 0x04)

#define RSC_DCM_CTL_HW                (rsc_reg_base + 0x08)
#define RSC_DCM_STAUS_HW              (rsc_reg_base + 0x0C)

#define RSC_INT_CTL_HW                (rsc_reg_base + 0x10)
#define RSC_INT_STATUS_HW             (rsc_reg_base + 0x14)
#define RSC_CTRL_HW                   (rsc_reg_base + 0x18)
#define RSC_SIZE_HW                   (rsc_reg_base + 0x1C)

#define RSC_SR_HW                     (rsc_reg_base + 0x20)
#define RSC_BR_HW                     (rsc_reg_base + 0x24)
#define RSC_MV_OFFSET_HW              (rsc_reg_base + 0x28)
#define RSC_GMV_OFFSET_HW             (rsc_reg_base + 0x2c)
#define RSC_PREPARE_MV_CTRL_HW        (rsc_reg_base + 0x30)
#define RSC_CAND_NUM_HW               (rsc_reg_base + 0x34)
#define RSC_EVEN_CAND_SEL_0_HW        (rsc_reg_base + 0x38)
#define RSC_EVEN_CAND_SEL_1_HW        (rsc_reg_base + 0x3c)
#define RSC_EVEN_CAND_SEL_2_HW        (rsc_reg_base + 0x40)
#define RSC_EVEN_CAND_SEL_3_HW        (rsc_reg_base + 0x44)
#define RSC_EVEN_CAND_SEL_4_HW        (rsc_reg_base + 0x48)
#define RSC_ODD_CAND_SEL_0_HW         (rsc_reg_base + 0x4C)
#define RSC_ODD_CAND_SEL_1_HW         (rsc_reg_base + 0x50)
#define RSC_ODD_CAND_SEL_2_HW         (rsc_reg_base + 0x54)
#define RSC_ODD_CAND_SEL_3_HW         (rsc_reg_base + 0x58)
#define RSC_ODD_CAND_SEL_4_HW         (rsc_reg_base + 0x5C)
#define RSC_RAND_HORZ_LUT_HW          (rsc_reg_base + 0x60)
#define RSC_RAND_VERT_LUT_HW          (rsc_reg_base + 0x64)
#define RSC_CURR_BLK_CTRL_HW          (rsc_reg_base + 0x68)
#define RSC_SAD_CTRL_HW               (rsc_reg_base + 0x6C)
#define RSC_SAD_EDGE_GAIN_CTRL_HW     (rsc_reg_base + 0x70)
#define RSC_SAD_CRNR_GAIN_CTRL_HW     (rsc_reg_base + 0x74)
#define RSC_STILL_STRIP_CTRL_0_HW     (rsc_reg_base + 0x78)
#define RSC_STILL_STRIP_CTRL_1_HW     (rsc_reg_base + 0x7C)
#define RSC_MV_PNLTY_CTRL_HW          (rsc_reg_base + 0x80)
#define RSC_ZERO_PNLTY_CTRL_HW        (rsc_reg_base + 0x84)
#define RSC_RAND_PNLTY_CTRL_HW        (rsc_reg_base + 0x88)
#define RSC_RAND_PNLTY_GAIN_CTRL_0_HW (rsc_reg_base + 0x8C)
#define RSC_RAND_PNLTY_GAIN_CTRL_1_HW (rsc_reg_base + 0x90)
#define RSC_TMPR_PNLTY_GAIN_CTRL_0_HW (rsc_reg_base + 0x94)
#define RSC_TMPR_PNLTY_GAIN_CTRL_1_HW (rsc_reg_base + 0x98)

#define RSC_IMGI_C_BASE_ADDR_HW       (rsc_reg_base + 0x9C)
#define RSC_IMGI_C_STRIDE_HW          (rsc_reg_base + 0xA0)
#define RSC_IMGI_P_BASE_ADDR_HW       (rsc_reg_base + 0xA4)
#define RSC_IMGI_P_STRIDE_HW          (rsc_reg_base + 0xA8)
#define RSC_MVI_BASE_ADDR_HW          (rsc_reg_base + 0xAC)
#define RSC_MVI_STRIDE_HW             (rsc_reg_base + 0xB0)
#define RSC_APLI_C_BASE_ADDR_HW       (rsc_reg_base + 0xB4)
#define RSC_APLI_P_BASE_ADDR_HW       (rsc_reg_base + 0xB8)
#define RSC_MVO_BASE_ADDR_HW          (rsc_reg_base + 0xBC)
#define RSC_MVO_STRIDE_HW             (rsc_reg_base + 0xC0)
#define RSC_BVO_BASE_ADDR_HW          (rsc_reg_base + 0xC4)
#define RSC_BVO_STRIDE_HW             (rsc_reg_base + 0xC8)

#define RSC_STA_0_HW                  (rsc_reg_base + 0x100)
#define RSC_DBG_INFO_00_HW            (rsc_reg_base + 0x120)
#define RSC_DBG_INFO_01_HW            (rsc_reg_base + 0x124)
#define RSC_DBG_INFO_02_HW            (rsc_reg_base + 0x128)
#define RSC_DBG_INFO_03_HW            (rsc_reg_base + 0x12C)
#define RSC_DBG_INFO_04_HW            (rsc_reg_base + 0x130)
#define RSC_DBG_INFO_05_HW            (rsc_reg_base + 0x134)
#define RSC_DBG_INFO_06_HW            (rsc_reg_base + 0x138)

#define RSC_SPARE_0_HW                (rsc_reg_base + 0x1F8)
#define RSC_SPARE_1_HW                (rsc_reg_base + 0x1FC)
#define RSC_DMA_DBG_HW                (rsc_reg_base + 0x7F4)
#define RSC_DMA_REQ_STATUS_HW         (rsc_reg_base + 0x7F8)
#define RSC_DMA_RDY_STATUS_HW         (rsc_reg_base + 0x7FC)

#define RSC_DMA_DMA_SOFT_RSTSTAT_HW   (rsc_reg_base + 0x800)
#define RSC_DMA_VERTICAL_FLIP_EN_HW   (rsc_reg_base + 0x804)
#define RSC_DMA_DMA_SOFT_RESET_HW     (rsc_reg_base + 0x808)
#define RSC_DMA_LAST_ULTRA_HW         (rsc_reg_base + 0x80C)
#define RSC_DMA_SPECIAL_FUN_HW        (rsc_reg_base + 0x810)
#define RSC_DMA_RSCO_BASE_ADDR_HW     (rsc_reg_base + 0x830)
#define RSC_DMA_RSCO_BASE_ADDR_2_HW   (rsc_reg_base + 0x834)
#define RSC_DMA_RSCO_OFST_ADDR_HW     (rsc_reg_base + 0x838)
#define RSC_DMA_RSCO_OFST_ADDR_2_HW   (rsc_reg_base + 0x83C)
#define RSC_DMA_RSCO_XSIZE_HW         (rsc_reg_base + 0x840)
#define RSC_DMA_RSCO_YSIZE_HW         (rsc_reg_base + 0x844)
#define RSC_DMA_RSCO_STRIDE_HW        (rsc_reg_base + 0x848)
#define RSC_DMA_RSCO_CON_HW           (rsc_reg_base + 0x84C)
#define RSC_DMA_RSCO_CON2_HW          (rsc_reg_base + 0x850)
#define RSC_DMA_RSCO_CON3_HW          (rsc_reg_base + 0x854)
#define RSC_DMA_RSCI_BASE_ADDR_HW     (rsc_reg_base + 0x890)
#define RSC_DMA_RSCI_BASE_ADDR_2_HW   (rsc_reg_base + 0x894)
#define RSC_DMA_RSCI_OFST_ADDR_HW     (rsc_reg_base + 0x898)
#define RSC_DMA_RSCI_OFST_ADDR_2_HW   (rsc_reg_base + 0x89C)
#define RSC_DMA_RSCI_XSIZE_HW         (rsc_reg_base + 0x8A0)
#define RSC_DMA_RSCI_YSIZE_HW         (rsc_reg_base + 0x8A4)
#define RSC_DMA_RSCI_STRIDE_HW        (rsc_reg_base + 0x8A8)
#define RSC_DMA_RSCI_CON_HW           (rsc_reg_base + 0x8AC)
#define RSC_DMA_RSCI_CON2_HW          (rsc_reg_base + 0x8B0)
#define RSC_DMA_RSCI_CON3_HW          (rsc_reg_base + 0x8B4)
#define RSC_DMA_DMA_ERR_CTRL_HW       (rsc_reg_base + 0x900)
#define RSC_DMA_RSCO_ERR_STAT_HW      (rsc_reg_base + 0x904)
#define RSC_DMA_RSCI_ERR_STAT_HW      (rsc_reg_base + 0x908)
#define RSC_DMA_DMA_DEBUG_ADDR_HW     (rsc_reg_base + 0x90C)
#define RSC_DMA_DMA_DEBUG_SEL_HW      (rsc_reg_base + 0x928)
#define RSC_DMA_DMA_BW_SELF_TEST_HW   (rsc_reg_base + 0x92C)

void rsc_init_handler(struct share_obj *in_data,struct share_obj *out_data);
void rsc_frame_handler(struct share_obj *in_data,struct share_obj *out_data);

struct v4l2_rsc_init_param {
    uint32_t    reg_base;
    uint32_t    reg_range;
} __attribute__ ((__packed__));

struct v4l2_rsc_img_plane_format {
    uint32_t    size;
    uint16_t    stride;
} __attribute__ ((__packed__));

struct rsc_img_pix_format {
    uint16_t        width;
    uint16_t        height;
    struct v4l2_rsc_img_plane_format plane_fmt[1];
} __attribute__ ((__packed__));

struct rsc_img_buffer {
    struct rsc_img_pix_format    format;
    uint32_t        iova[1];
} __attribute__ ((__packed__));

struct rsc_meta_buffer {
    uint64_t    va;    /* Used by APMCU access */
    uint32_t    pa;    /* Used by CM4 access */
    uint32_t    iova;  /* Used by IOMMU HW access */
} __attribute__ ((__packed__));

struct v4l2_rsc_frame_param {
    struct rsc_img_buffer     pre_rrzo_in[1];
    struct rsc_img_buffer     cur_rrzo_in[1];
    struct rsc_meta_buffer    meta_out;
    uint32_t                  tuning_data[MTK_ISP_CTX_RSC_TUNING_DATA_NUM];
    uint32_t                  frame_id;
} __attribute__ ((__packed__));

struct rsc_tuning_config {
    uint32_t        ctrl;
    uint32_t        size;
    uint32_t        cur_rrzo_iova;
    uint32_t        pre_rrzo_iova;
    uint32_t        mv_i_iova;
    uint32_t        mv_i_stride;
    uint32_t        mv_o_iova;
    uint32_t        mv_o_stride;
    uint32_t        bv_o_iova;
    uint32_t        bv_o_stride;
    uint32_t        mv_offset;
    uint32_t        gmv_offset;
    uint32_t        cand_num;
    uint32_t        rand_horz_lut;
    uint32_t        rand_vert_lut;
    uint32_t        sad_ctrl;
    uint32_t        sad_edge_gain_ctrl;
    uint32_t        sad_crnr_gain_ctrl;
    uint32_t        still_strip_ctrl0;
    uint32_t        still_strip_ctrl1;
    uint32_t        rand_pnlty_ctrl;
    uint32_t        rand_pnlty_gain_ctrl0;
    uint32_t        rand_pnlty_gain_ctrl1;
} __attribute__ ((__packed__));



#endif /* _RSC_HANDLER_H_ */

