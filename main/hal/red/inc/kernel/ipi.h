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

#ifndef __IPI_H
#define __IPI_H

#define IPI_INST_CNT      MODULE_MAX_ID
#define SHARE_BUF_SIZE    288

enum hcp_id {
    HCP_INIT_ID = 0,
    HCP_ISP_CMD_ID,
    HCP_ISP_FRAME_ID,
    HCP_DIP_INIT_ID,
    HCP_DIP_FRAME_ID,
    HCP_DIP_HW_TIMEOUT_ID,
    HCP_FD_CMD_ID,
    HCP_FD_FRAME_ID,
    HCP_RSC_INIT_ID,
    HCP_RSC_FRAME_ID,
    HCP_MAX_ID,
};

enum module_id {
    MODULE_ISP = 0,
    MODULE_DIP,
    MODULE_FD,
    MODULE_RSC,
    MODULE_MAX_ID,
};

struct share_obj {
    signed int id;
    unsigned int len;
    unsigned char share_buf[SHARE_BUF_SIZE];
};

enum ipi_status
{
    ERROR = -1,
    DONE = 0,
    BUSY,
};

typedef void(*ipi_handler_t)(struct share_obj *data);

struct ipi_ctx {
    int fd;
    int cmd;
};

struct ipi_desc{
    ipi_handler_t handler;
    const char  *name;
};

void ipi_init(void);
void ipi_handler(struct share_obj *obj);

enum ipi_status ipi_registration(enum hcp_id id, ipi_handler_t handler, const char *name);
enum ipi_status ipi_complete(enum hcp_id id, void *buf, uint len, unsigned int wait);
enum ipi_status ipi_notify(enum hcp_id id, void *buf, uint len, unsigned int wait);
enum ipi_status ipi_wakeup(enum hcp_id id, void *buf, uint len, unsigned int wait);
enum ipi_status ipi_status(enum hcp_id id);
void wdt_lock(int id);
void wdt_unlock(int id);
void wdt_trigger(void);
void wdt_wait(void);

#endif
