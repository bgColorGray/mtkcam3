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
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include <cutils/log.h>

#include "red.h"
#include "thread.h"
#include "ipi.h"
#include "app.h"

//#include <mtkcam/def/common.h>

#define  LOG_TAG "RED"
#undef   DBG_LOG_TAG                    // Decide a Log TAG for current file.
#define  DBG_LOG_TAG        LOG_TAG
#include "log.h"                        // Note: DBG_LOG_TAG/LEVEL will be used in header file, so header must be included after definition.
DECLARE_DBG_LOG_VARIABLE(red_drv);

// Clear previous define, use our own define.
#undef LOG_VRB
#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR
#undef LOG_AST
#define LOG_VRB(fmt, arg...)        do { if (red_drv_DbgLogEnable_VERBOSE) { BASE_LOG_VRB(fmt, ##arg); } } while(0)
#define LOG_DBG(fmt, arg...)        do { if (red_drv_DbgLogEnable_DEBUG  ) { BASE_LOG_DBG(fmt, ##arg); } } while(0)
#define LOG_INF(fmt, arg...)        do { if (red_drv_DbgLogEnable_INFO   ) { BASE_LOG_INF(fmt, ##arg); } } while(0)
#define LOG_WRN(fmt, arg...)        do { if (red_drv_DbgLogEnable_WARN   ) { BASE_LOG_WRN(fmt, ##arg); } } while(0)
#define LOG_ERR(fmt, arg...)        do { if (red_drv_DbgLogEnable_ERROR  ) { BASE_LOG_ERR(fmt, ##arg); } } while(0)
#define LOG_AST(cond, fmt, arg...)  do { if (red_drv_DbgLogEnable_ASSERT ) { BASE_LOG_AST(cond, fmt, ##arg); } } while(0)

static struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct share_obj obj;
} rcv = {
    .cond = PTHREAD_COND_INITIALIZER,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

static pthread_t thread_rcv[MODULE_MAX_ID];
volatile static int g_fg_exit = 0;
extern struct ipi_ctx g_hcp_ctx;

static unsigned int g_hcp_fd = 0;
void*  g_map_addr[MMAP_END_ID];

static void *receiver(void *arg)
{
    int ret = 0;
    struct share_obj in_data;

    while (1) {
        memset(&in_data, 0, sizeof(struct share_obj));
        in_data.id = (int)(long)arg;

        ret = ioctl(g_hcp_fd, RED_GET_OBJECT, &in_data);
        if (g_fg_exit) {
            break;
        }

        if (ret != 0) {
            usleep(1000);
            if (g_fg_exit) {
                break;
            }
        } else {
            ipi_handler(&in_data);
        }
    }

    //LOG_INF("[RED] receiver thread breaked\n");

    return NULL;
}

void red_init()
{
    int ret;

    g_hcp_fd = open(PATH_DEV_HCP, O_RDWR);
    LOG_DBG("open mtk_hcp and fd:%d\n",g_hcp_fd);
    g_hcp_ctx.fd = g_hcp_fd;

    ipi_init();

    /* Create receiver thread  */
    /* The SCHED_FIFO scheduling class is a longstanding, POSIX-specified
     * realtime feature. Processes in this class are given the CPU for as
     * long as they want it, subject only to the needs for higher-priority
     * realtime processes.
     */
    /*
      * Priority of a process goes from 0..MAX_PRIO-1, valid RT
      * priority is 0..MAX_RT_PRIO-1, and SCHED_NORMAL/SCHED_BATCH
      * tasks are in the range MAX_RT_PRIO..MAX_PRIO-1. Priority
      * values are inverted: lower p->prio value means higher priority.
      */
    //ret = create_rt_thread(&thread_rcv[0], receiver, (void *)(long)(MODULE_ISP), SCHED_FIFO, 99);
    //ret = create_rt_thread(&thread_rcv[1], receiver, (void *)(long)(MODULE_DIP), SCHED_FIFO, 99);
    //ret = create_rt_thread(&thread_rcv[2], receiver, (void *)(long)(MODULE_FD) , SCHED_FIFO, 99);
    ret = create_rt_thread(&thread_rcv[3], receiver, (void *)(long)(MODULE_RSC), SCHED_FIFO, 99);

    assert(ret == 0);

    apps_init();
}

void startRED()
{
    LOG_DBG("[RED] begin of startRED\n");

    red_init();

    LOG_DBG("[RED] end of startRED\n");

    return ;
}

/**
 * get_mmap_addr - get VA address for reserved memory, HW registers, etc.
 *
 * @mmap_module_id:    @see enum mmap_module_id
 * @return:    VA address
 */
void* get_mmap_addr(int mmap_module_id)
{
    LOG_DBG("[RED][%s] mmap_module_id=%d",  __FUNCTION__, mmap_module_id);
    if (mmap_module_id < 0 || mmap_module_id >= MMAP_END_ID)
    {
        LOG_DBG("mmap_module_id(%d) is invalid", mmap_module_id);
        return 0;
    }

    return g_map_addr[mmap_module_id];
}

