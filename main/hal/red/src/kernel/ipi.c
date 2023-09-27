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

#include <stdio.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "type.h"
#include "ipi.h"
#include "red.h"
#include "mutex.h"

#define  LOG_TAG "RED_IPI"
#undef   DBG_LOG_TAG                    // Decide a Log TAG for current file.
#define  DBG_LOG_TAG        LOG_TAG
#include "log.h"                        // Note: DBG_LOG_TAG/LEVEL will be used in header file, so header must be included after definition.
DECLARE_DBG_LOG_VARIABLE(RED_IPI);

// Clear previous define, use our own define.
#undef LOG_VRB
#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR
#undef LOG_AST
#define LOG_VRB(fmt, arg...)        if (RED_IPI_DbgLogEnable_VERBOSE) { BASE_LOG_VRB(fmt, ##arg); }
#define LOG_DBG(fmt, arg...)        if (RED_IPI_DbgLogEnable_DEBUG  ) { BASE_LOG_DBG(fmt, ##arg); }
#define LOG_INF(fmt, arg...)        if (RED_IPI_DbgLogEnable_INFO   ) { BASE_LOG_INF(fmt, ##arg); }
#define LOG_WRN(fmt, arg...)        if (RED_IPI_DbgLogEnable_WARN   ) { BASE_LOG_WRN(fmt, ##arg); }
#define LOG_ERR(fmt, arg...)        if (RED_IPI_DbgLogEnable_ERROR  ) { BASE_LOG_ERR(fmt, ##arg); }
#define LOG_AST(fmt, arg...)        if (RED_IPI_DbgLogEnable_ASSERT ) { BASE_LOG_AST(fmt, ##arg); }


struct ipi_desc ipi_desc[HCP_MAX_ID];
static sem_t work_sem[IPI_INST_CNT];
static sem_t done_sem[IPI_INST_CNT];
static pthread_mutex_t mutex[IPI_INST_CNT]={PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER};
static mutex_t wdt_mutex[IPI_INST_CNT];
static sem_t wdt_enable_sem;
static struct share_obj obj[IPI_INST_CNT];
static bool busy[IPI_INST_CNT];
static bool wait_sync[IPI_INST_CNT];
struct ipi_ctx g_hcp_ctx; /* initialized in daemon (e.g. red.cpp) */
enum ipi_status ipi_send(unsigned int cmd, enum hcp_id id, void *buf, uint len, unsigned int wait);

void ipi_init(void)
{
    int i;

    for (i = 0; i < IPI_INST_CNT; i++) {
        sem_init(&work_sem[i], 0, 0);
        sem_init(&done_sem[i], 0, 0);
        mutex_init(&wdt_mutex[i]);
    }
    sem_init(&wdt_enable_sem, 0, 0);
}

inline int hcp_id_to_inst_id(int id)
{
    int module_id = -1;
    switch(id) {
    case HCP_ISP_CMD_ID:
    case HCP_ISP_FRAME_ID:
        module_id = MODULE_ISP;
        break;
    case HCP_DIP_INIT_ID:
    case HCP_DIP_FRAME_ID:
    case HCP_DIP_HW_TIMEOUT_ID:
        module_id = MODULE_DIP;
        break;
    case HCP_FD_CMD_ID:
    case HCP_FD_FRAME_ID:
        module_id = MODULE_FD;
        break;
    case HCP_RSC_INIT_ID:
    case HCP_RSC_FRAME_ID:
        module_id = MODULE_RSC;
        break;
    default:
        break;
    }
    LOG_DBG("module_id=%d \n",module_id);
    return module_id;
}

void ipi_handler(struct share_obj *obj)
{
    LOG_INF("ipi_handler- id:%d\n", obj->id);
    if (ipi_desc[obj->id].handler != NULL) {
        ipi_desc[obj->id].handler(obj);
    } else {
        LOG_DBG("Get ipi_id: %d obj handler is NULL\n", obj->id);
    }
}

void wdt_lock(int id)
{
    while (mutex_acquire(&wdt_mutex[hcp_id_to_inst_id(id)]) != 0);
}

void wdt_unlock(int id)
{
    while (mutex_release(&wdt_mutex[hcp_id_to_inst_id(id)]) != 0);
}

int ipi_wdt_check(enum hcp_id id)
{
    int i, ret;
    i = hcp_id_to_inst_id(id);
    ret = mutex_acquire_timeout(&wdt_mutex[i], 5000); // 5 secend timeout
    if (!ret) {
        while (mutex_release(&wdt_mutex[i]) != 0);
    }
    return ret;
}

void wdt_trigger(void)
{
    sem_post(&wdt_enable_sem);
}

void wdt_wait(void)
{
    sem_wait(&wdt_enable_sem);
}

enum ipi_status ipi_registration(enum hcp_id id, ipi_handler_t handler, const char *name)
{
    assert(id >= HCP_INIT_ID && id < HCP_MAX_ID);
    assert(handler != NULL);

    LOG_INF("ipi_registration- id:%d\n", id);
    ipi_desc[id].handler = handler;
    ipi_desc[id].name = name;

    return DONE;
}

enum ipi_status ipi_complete(enum hcp_id id, void *buf, uint len, unsigned int wait)
{
    return ipi_send(RED_COMPLETE, id, buf, len, wait);
}

enum ipi_status ipi_notify(enum hcp_id id, void *buf, uint len, unsigned int wait)
{
    return ipi_send(RED_NOTIFY, id, buf, len, wait);
}

enum ipi_status ipi_wakeup(enum hcp_id id, void *buf, uint len, unsigned int wait)
{
    return ipi_send(RED_WAKEUP, id, buf, len, wait);
}

enum ipi_status ipi_send(unsigned int cmd, enum hcp_id id, void *buf, uint len, unsigned int wait)
{
    int i = 0;
    i = hcp_id_to_inst_id(id);
    assert(id >= HCP_INIT_ID && id < HCP_MAX_ID);
    assert(buf != NULL);
    assert(len <= sizeof(obj[i].share_buf));

    LOG_INF("cmd:%d, id:%d, length:%d, wait:%d \n", cmd, id, len, wait);

    while (pthread_mutex_lock(&mutex[i]) != 0);
    if (busy[i]) {
        //ipi_log("send id = %d return\n", id);
        while (pthread_mutex_unlock(&mutex[i]) != 0);
        return BUSY;
    } else {
        busy[i] = true;
    }

    obj[i].id = id;
    obj[i].len = len;
    memcpy(obj[i].share_buf, buf, len);
    wait_sync[i] = wait ? true : false;

    if (wait_sync[i]) {
        /*
         * For synchronous call of ipi_send, skip sender thread in
         * daemon and skip 2 semaphores synchronization, which decreases
         * the latency.
         */
        int ret;
        ret = ioctl(g_hcp_ctx.fd, cmd, &obj[i]);
        assert(ret != -1);
        busy[i] = false;
    } else {
        sem_post(&work_sem[i]);

        if (wait_sync[i]) {
            sem_wait(&done_sem[i]);
            busy[i] = false;
        }
    }

    while (pthread_mutex_unlock(&mutex[i]) != 0);

    return DONE;
}

