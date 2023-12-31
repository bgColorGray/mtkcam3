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

#include <stddef.h>

#include "app.h"

#define  LOG_TAG "RED_APP"
#undef   DBG_LOG_TAG                    // Decide a Log TAG for current file.
#define  DBG_LOG_TAG        LOG_TAG
#include "log.h"                        // Note: DBG_LOG_TAG/LEVEL will be used in header file, so header must be included after definition.
DECLARE_DBG_LOG_VARIABLE(RED_APP);

// Clear previous define, use our own define.
#undef LOG_VRB
#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR
#undef LOG_AST
#define LOG_VRB(fmt, arg...)        if (RED_APP_DbgLogEnable_VERBOSE) { BASE_LOG_VRB(fmt, ##arg); }
#define LOG_DBG(fmt, arg...)        if (RED_APP_DbgLogEnable_DEBUG  ) { BASE_LOG_DBG(fmt, ##arg); }
#define LOG_INF(fmt, arg...)        if (RED_APP_DbgLogEnable_INFO   ) { BASE_LOG_INF(fmt, ##arg); }
#define LOG_WRN(fmt, arg...)        if (RED_APP_DbgLogEnable_WARN   ) { BASE_LOG_WRN(fmt, ##arg); }
#define LOG_ERR(fmt, arg...)        if (RED_APP_DbgLogEnable_ERROR  ) { BASE_LOG_ERR(fmt, ##arg); }
#define LOG_AST(fmt, arg...)        if (RED_APP_DbgLogEnable_ASSERT ) { BASE_LOG_AST(fmt, ##arg); }

//extern struct app_descriptor _app_isp_service;
//extern struct app_descriptor _app_dip_service;
//extern struct app_descriptor _app_fd_service;
extern struct app_descriptor _app_rsc_service;


const struct app_descriptor *apps[] = {
    //&_app_isp_service,
    //&_app_dip_service,
    //&_app_fd_service,
    &_app_rsc_service,
    NULL
};

/* one time setup */
void apps_init(void)
{
    const struct app_descriptor *app;
    int i;

    /* call all the init routines */
    for (i = 0; apps[i] != NULL; i++) {
        app = apps[i];
        if (app->init) {
            app->init(app);
        }
    }
}

int app_thread_entry(void *arg)
{
    const struct app_descriptor *app = (const struct app_descriptor *)arg;

    app->entry(app, NULL);

    return 0;
}
