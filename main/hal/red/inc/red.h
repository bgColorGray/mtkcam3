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

#ifndef _RED_H_
#define _RED_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#include "ipi.h"

#define PATH_DEV_HCP    "/dev/mtk_hcp"
#define DEV_NAME        "RED"

/*
 * define magic number for reserved memory to use in mmap function.
 */
#define START_ISP_MEM_ADDR      0x12345000
#define ISP_RESERVERD_MEM_SIZE  0x200000
#define START_DIP_MEM_FOR_HW_ADDR 0x12346000
#define DIP_MEM_FOR_HW_SIZE  0x400000
#define START_MDP_MEM_ADDR      0x12347000
#define MDP_RESERVERD_MEM_SIZE  0x400000
#define START_FD_MEM_ADDR       0x12348000
#define FD_RESERVERD_MEM_SIZE   0x100000
#define START_DIP_MEM_FOR_SW_ADDR 0x12349000
#define DIP_MEM_FOR_SW_SIZE 0x100000


/*
 * define module register mmap information
 */
#define ISP_UNI_A_BASE_HW       0x1A003000
#define ISP_A_BASE_HW           0x1A004000
#define ISP_B_BASE_HW           0x1A006000
#define ISP_C_BASE_HW           0x1A008000
#define ISP_REG_RANGE    0x2000

#define DIP_REG_RANGE    0xC000
#define DIP_BASE_HW      0x15021000
#define FD_REG_RANGE     0x1000
#define FD_BASE_HW       0x1502B000

enum {
    RED_INIT        = _IOWR('J', 0, struct share_obj),
    RED_GET_OBJECT  = _IOWR('J', 1, struct share_obj),
    RED_NOTIFY      = _IOWR('J', 2, struct share_obj),
    RED_COMPLETE    = _IOWR('J', 3, struct share_obj),
    RED_WAKEUP      = _IOWR('J', 4, struct share_obj),
};

/**
 * mmap module ID definition
 */
enum mmap_module_id {
    ISP_UNI_REGA,
    ISP_REGA,
    ISP_REGB,
    ISP_REGC,
    ISP_RESERVERD_MEMORY,
    DIP_REG,
    DIP_RESERVERD_MEMORY_FOR_HW,
    DIP_RESERVERD_MEMORY_FOR_SW,
    MDP_REG,
    MDP_RESERVERD_MEMORY,
    FD_REG,
    FD_RESERVERD_MEMORY,
    SHARE_MEMORY,
    MMAP_END_ID,
};

void startRED();

void mdp_init();

/******************************************
 * RED Utility Functions
 ******************************************/

 #ifdef __cplusplus
extern "C" {
#endif

/**
 * get_mmap_addr - get VA address for reserved memory, HW registers, etc.
 *
 * @mmap_module_id:    @see enum mmap_module_id
 * @return:    VA address
 */
void* get_mmap_addr(int mmap_module_id);

#ifdef __cplusplus
}
#endif

#endif /* _RED_H_ */

