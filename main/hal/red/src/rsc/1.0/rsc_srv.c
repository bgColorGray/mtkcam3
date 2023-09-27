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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "thread.h"
#include "ipi.h"
#include "event.h"
#include "rsc_srv.h"
#include "rsc_handler.h"

#define  LOG_TAG "RSCSRV"
#undef   DBG_LOG_TAG                    // Decide a Log TAG for current file.
#define  DBG_LOG_TAG        LOG_TAG
#include "log.h"                        // Note: DBG_LOG_TAG/LEVEL will be used in header file, so header must be included after definition.
DECLARE_DBG_LOG_VARIABLE(RSCSRV);

// Clear previous define, use our own define.
#undef RSC_LOGE
#undef RSC_LOGW
#undef RSC_LOGI
#undef RSC_LOGD
#define RSC_LOGE(fmt, arg...)        if (RSCSRV_DbgLogEnable_VERBOSE) { BASE_LOG_VRB("[ISP][RSC]ERR: " fmt, ##arg); }
#define RSC_LOGW(fmt, arg...)        if (RSCSRV_DbgLogEnable_DEBUG  ) { BASE_LOG_DBG("[ISP][RSC]WARN: " fmt, ##arg); }
#define RSC_LOGI(fmt, arg...)        if (RSCSRV_DbgLogEnable_INFO   ) { BASE_LOG_INF("[ISP][RSC]INFO: " fmt, ##arg); }
#define RSC_LOGD(fmt, arg...)        if (RSCSRV_DbgLogEnable_WARN   ) { BASE_LOG_WRN("[ISP][RSC]DBG: " fmt, ##arg); }

static struct rsc_service s;
static void rsc_service_init(const struct app_descriptor *app);
static void rsc_service_entry(const struct app_descriptor *app, void *args);

struct app_descriptor _app_rsc_service = {.magic = MAGIC,
        .name = "rsc_service",
        .init = rsc_service_init,
        .entry = rsc_service_entry,
        .flags = 1};

static void rsc_ipi_handler(struct share_obj *obj)
{
    enter_critical_section();
    s.type = obj->id;
    memcpy(s.msg, obj, sizeof(struct share_obj));
    RSC_LOGW("rsc_ipi_handler with id:%d \n",obj->id);

    exit_critical_section();
    event_signal(&s.event, false);
}

static void rsc_service_init(const struct app_descriptor *app)
{
    /* register IPI handler decoder */
    ipi_registration(HCP_RSC_INIT_ID, rsc_ipi_handler, "rsc_init");
    ipi_registration(HCP_RSC_FRAME_ID, rsc_ipi_handler, "rsc_frame");

    s.handler = rsc_msg_handler;

    event_init(&s.event, false, EVENT_FLAG_AUTOUNSIGNAL);
    _app_rsc_service.name = "rsc";
    thread_resume(thread_create(THREAD_RSC, _app_rsc_service.name,
                                &app_thread_entry, (void *)&_app_rsc_service, HIGH_PRIORITY));
}

static void rsc_service_entry(const struct app_descriptor *app, void *args)
{
    while(1) {
        event_wait(&s.event);
        if (!strcmp((char*)s.msg, "SIGTERM")) {
            wdt_trigger();
            break;
        }

        if (NULL != s.handler) {
            wdt_lock(s.type);
            wdt_trigger();
        enter_critical_section();
            s.handler(s.msg, s.type);
        exit_critical_section();
            wdt_unlock(s.type);
        }
    }
}

void rsc_msg_handler(void *msg, unsigned int hcp_id)
{
    RSC_LOGW("rsc_msg_handler with id:%d \n", hcp_id);

    enum ipi_status status = ERROR;
    struct share_obj out_obj;
    int i =0;

    memset(&out_obj, 0, sizeof(struct share_obj));
    out_obj.id = hcp_id;

    switch (hcp_id) {
    case HCP_RSC_INIT_ID:
        rsc_init_handler((struct share_obj *)msg, &out_obj);
        break;
    case HCP_RSC_FRAME_ID:
        rsc_frame_handler((struct share_obj *)msg, &out_obj);
        break;
    default:
        break;
    }

    ipi_complete((enum hcp_id)out_obj.id, out_obj.share_buf, out_obj.len, 1);
    return;
}

