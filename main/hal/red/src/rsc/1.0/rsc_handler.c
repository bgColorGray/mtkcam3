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

#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include "rsc_handler.h"
#include <cutils/log.h>
//#include <mtkcam/def/common.h>

#include "rsc_handler.h"

#define  LOG_TAG "RSCHandler"
#undef   DBG_LOG_TAG                    // Decide a Log TAG for current file.
#define  DBG_LOG_TAG        LOG_TAG
#include "log.h"                        // Note: DBG_LOG_TAG/LEVEL will be used in header file, so header must be included after definition.
DECLARE_DBG_LOG_VARIABLE(RSCHandler);

// Clear previous define, use our own define.
#undef LOG_VRB
#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR
#undef LOG_AST
#define LOG_VRB(fmt, arg...)        do { if (RSCHandler_DbgLogEnable_VERBOSE) { BASE_LOG_VRB(fmt, ##arg); } } while(0)
#define LOG_DBG(fmt, arg...)        do { if (RSCHandler_DbgLogEnable_DEBUG  ) { BASE_LOG_DBG(fmt, ##arg); } } while(0)
#define LOG_INF(fmt, arg...)        do { if (RSCHandler_DbgLogEnable_INFO   ) { BASE_LOG_INF(fmt, ##arg); } } while(0)
#define LOG_WRN(fmt, arg...)        do { if (RSCHandler_DbgLogEnable_WARN   ) { BASE_LOG_WRN(fmt, ##arg); } } while(0)
#define LOG_ERR(fmt, arg...)        do { if (RSCHandler_DbgLogEnable_ERROR  ) { BASE_LOG_ERR(fmt, ##arg); } } while(0)
#define LOG_AST(cond, fmt, arg...)  do { if (RSCHandler_DbgLogEnable_ASSERT ) { BASE_LOG_AST(cond, fmt, ##arg); } } while(0)

void rsc_init_handler(struct share_obj *in_data,struct share_obj *out_data)
{
    struct v4l2_rsc_init_param *info =
            (struct v4l2_rsc_init_param *)in_data->share_buf;

    rsc_reg_base = info->reg_base;
    LOG_INF("rsc_init_handler - reg_base:%d \n", rsc_reg_base);
    return;
}

void rsc_frame_handler(struct share_obj *in_data, struct share_obj *out_data)
{

    struct v4l2_rsc_frame_param rsc_param;
    struct rsc_tuning_config tuning_data;
    unsigned int rsc_config_param[30][2];
    unsigned int height = 0;
    unsigned int width = 0;
    unsigned int height_p = 0;
    unsigned int width_p = 0;


    out_data->id = in_data->id;
    memset(&rsc_param, 0, sizeof(rsc_param));
    memcpy(&rsc_param, in_data->share_buf, in_data->len);
    LOG_DBG("rsc_frame_handler - data size:%d \n", in_data->len);
    LOG_DBG("rsc_frame_handler - cur rrzo width:%d height:%d stride:%d\n",
            rsc_param.cur_rrzo_in[0].format.width,
            rsc_param.cur_rrzo_in[0].format.height,
            rsc_param.cur_rrzo_in[0].format.plane_fmt[0].stride);
    LOG_DBG("rsc_frame_handler - pre rrzo width:%d height:%d stride:%d\n",
            rsc_param.pre_rrzo_in[0].format.width,
            rsc_param.pre_rrzo_in[0].format.height,
            rsc_param.cur_rrzo_in[0].format.plane_fmt[0].stride);

    memcpy(&tuning_data, (void *)rsc_param.tuning_data,
            sizeof(struct rsc_tuning_config));

    width = rsc_param.cur_rrzo_in[0].format.width;
    height = rsc_param.cur_rrzo_in[0].format.height -1;
    width_p = rsc_param.pre_rrzo_in[0].format.width;
    height_p = rsc_param.pre_rrzo_in[0].format.height -1;

    rsc_config_param[0][0] = RSC_INT_CTL_HW;
    rsc_config_param[0][1] = 0x1;
    rsc_config_param[1][0] = RSC_CTRL_HW;
    rsc_config_param[1][1] = tuning_data.ctrl;
    rsc_config_param[2][0] = RSC_SIZE_HW;
    rsc_config_param[2][1] = tuning_data.size;
    rsc_config_param[3][0] = RSC_APLI_C_BASE_ADDR_HW;
    rsc_config_param[3][1] =
            rsc_param.cur_rrzo_in[0].iova[0] + width*height;
    rsc_config_param[4][0] = RSC_APLI_P_BASE_ADDR_HW;
    rsc_config_param[4][1] =
            rsc_param.pre_rrzo_in[0].iova[0] + width_p*height_p;
    rsc_config_param[5][0] = RSC_IMGI_C_BASE_ADDR_HW;
    rsc_config_param[5][1] = rsc_param.cur_rrzo_in[0].iova[0];
    rsc_config_param[6][0] = RSC_IMGI_P_BASE_ADDR_HW;
    rsc_config_param[6][1] = rsc_param.pre_rrzo_in[0].iova[0];
    rsc_config_param[7][0] = RSC_IMGI_C_STRIDE_HW;
    rsc_config_param[7][1] =
            rsc_param.cur_rrzo_in[0].format.plane_fmt[0].stride;
    rsc_config_param[8][0] = RSC_IMGI_P_STRIDE_HW;
    rsc_config_param[8][1] =
            rsc_param.pre_rrzo_in[0].format.plane_fmt[0].stride;
    rsc_config_param[9][0] = RSC_MVI_BASE_ADDR_HW;
    rsc_config_param[9][1] = tuning_data.mv_i_iova;
    rsc_config_param[10][0] = RSC_MVI_STRIDE_HW;
    rsc_config_param[10][1] = tuning_data.mv_i_stride;
    rsc_config_param[11][0] = RSC_MVO_BASE_ADDR_HW;
    rsc_config_param[11][1] = tuning_data.mv_o_iova;
    rsc_config_param[12][0] = RSC_MVO_STRIDE_HW;
    rsc_config_param[12][1] = tuning_data.mv_o_stride;
    rsc_config_param[13][0] = RSC_BVO_BASE_ADDR_HW;
    rsc_config_param[13][1] = tuning_data.bv_o_iova;
    rsc_config_param[14][0] = RSC_BVO_STRIDE_HW;
    rsc_config_param[14][1] = tuning_data.bv_o_stride;
    rsc_config_param[15][0] = RSC_MV_OFFSET_HW;
    rsc_config_param[15][1] = tuning_data.mv_offset;
    rsc_config_param[16][0] = RSC_GMV_OFFSET_HW;
    rsc_config_param[16][1] = tuning_data.gmv_offset;
    rsc_config_param[17][0] = RSC_CAND_NUM_HW;
    rsc_config_param[17][1] = tuning_data.cand_num;
    rsc_config_param[18][0] = RSC_RAND_HORZ_LUT_HW;
    rsc_config_param[18][1] = tuning_data.rand_horz_lut;
    rsc_config_param[19][0] = RSC_RAND_VERT_LUT_HW;
    rsc_config_param[19][1] = tuning_data.rand_vert_lut;
    rsc_config_param[20][0] = RSC_SAD_CTRL_HW;
    rsc_config_param[20][1] = tuning_data.sad_ctrl;
    rsc_config_param[21][0] = RSC_SAD_EDGE_GAIN_CTRL_HW;
    rsc_config_param[21][1] = tuning_data.sad_edge_gain_ctrl;
    rsc_config_param[22][0] = RSC_SAD_CRNR_GAIN_CTRL_HW;
    rsc_config_param[22][1] = tuning_data.sad_crnr_gain_ctrl;
    rsc_config_param[23][0] = RSC_STILL_STRIP_CTRL_0_HW;
    rsc_config_param[23][1] = tuning_data.still_strip_ctrl0;
    rsc_config_param[24][0] = RSC_STILL_STRIP_CTRL_1_HW;
    rsc_config_param[24][1] = tuning_data.still_strip_ctrl1;
    rsc_config_param[25][0] = RSC_RAND_PNLTY_CTRL_HW;
    rsc_config_param[25][1] = tuning_data.rand_pnlty_ctrl;
    rsc_config_param[26][0] = RSC_RAND_PNLTY_GAIN_CTRL_0_HW;
    rsc_config_param[26][1] = tuning_data.rand_pnlty_gain_ctrl0;
    rsc_config_param[27][0] = RSC_RAND_PNLTY_GAIN_CTRL_1_HW;
    rsc_config_param[27][1] = tuning_data.rand_pnlty_gain_ctrl1;
    rsc_config_param[28][0] = RSC_START_HW;
    rsc_config_param[28][1] = 0x1;
    rsc_config_param[29][0] = RSC_START_HW;
    rsc_config_param[29][1] = 0x0;

    for (int i=0; i<30; i++) {
        for (int j=0;j<2;j++) {
            LOG_DBG("rsc_frame_handler - config_param[%d][%d] = 0x%x\n",
                    i, j, rsc_config_param[i][j]);
        }
    }

    out_data->len = sizeof(unsigned int)*30*2;
    memcpy(out_data->share_buf, rsc_config_param, out_data->len);
    return;
}

