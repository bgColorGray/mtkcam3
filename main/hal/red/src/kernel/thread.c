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

#include <sys/prctl.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "thread.h"
#include "err.h"


/*
thread_create
thread_resume
*/

struct thread {
    pthread_t thread;
    pthread_mutex_t mutex;

    /* entry point */
    thread_start_routine entry;
    void *arg;

    char name[16];
};

static thread_t threads[NR_THREAD];

static void thread_suspend(thread_t *thread)
{
    while (pthread_mutex_lock(&thread->mutex) != 0);
}

status_t thread_resume(thread_t *thread)
{
    while (pthread_mutex_unlock(&thread->mutex) != 0);
    return NO_ERROR;
}

static void *thread_entry(void *arg)
{
    thread_t *thread = (thread_t *)(arg);

    prctl(PR_SET_NAME, thread->name);

    thread_suspend(thread);
    thread->entry(thread->arg);

    thread->thread = (pthread_t)(0);
    return NULL;
}

thread_t *thread_create(const unsigned int id, const char *name, thread_start_routine entry, void *arg, unsigned int priority)
{
    thread_t *thread = &threads[id];
    int i;

    pthread_mutex_init(&thread->mutex, NULL);
    pthread_mutex_lock(&thread->mutex);

    strncpy(thread->name, name, sizeof(thread->name));
    thread->name[sizeof(thread->name)-1] = '\0';
    thread->entry = entry;
    thread->arg = arg;

    i = pthread_create(&thread->thread, NULL, thread_entry, thread);
    assert(i == 0);

    return thread;
}

void thread_yield(void)
{
    sched_yield();
}

/*
enter_critical_section
exit_critical_section
*/

static struct {
    pthread_mutex_t mutex;
    pthread_t thread;
    int selfcount;
} crit = {
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

void enter_critical_section(void)
{
    pthread_t thread = pthread_self();
    if (!pthread_equal(crit.thread, thread)) {
        while (pthread_mutex_lock(&crit.mutex) != 0);
        assert(crit.selfcount == 0);
        crit.thread = thread;
    }
    crit.selfcount++;
}

void exit_critical_section(void)
{
    pthread_t thread = pthread_self();
    assert(pthread_equal(crit.thread, thread));
    crit.selfcount--;
    if (crit.selfcount == 0) {
        crit.thread = (pthread_t)(0);
        while (pthread_mutex_unlock(&crit.mutex) != 0);
    }
}

int create_rt_thread(pthread_t *pth, void*(*func)(void *), void *arg,
             int policy, int prio)
{
    int ret = 0, fail = 0;
    struct sched_param schedp;
    pthread_attr_t attr;
    pthread_attr_t *pattr = &attr;

    // The default policy is SCHED_NORMAL (0), priority is 0.
    // sched_getparam(getpid(), &schedp);
    // pthread_getschedparam(pthread_self(), &ret, &schedp);

    pthread_attr_init(&attr);
    /* safe to get existing scheduling param */
    pthread_attr_getschedparam(&attr, &schedp);
#ifndef __ANDROID__
    /*
      * PTHREAD_EXPLICIT_SCHED: Specifies that the scheduling policy and
      * associated attributes are to be set to the corresponding values from
      * this attribute object.android no support this api.
      */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        fprintf(stderr, "[red] pthread_attr_setinheritsched\n");
        fail = 1;
    }
#endif
    ret = pthread_attr_setschedpolicy(&attr, policy);
    if (ret) {
        fprintf(stderr, "[red] pthread_attr_setschedpolicy\n");
        fail = 1;
    }

    schedp.sched_priority = prio;
    ret = pthread_attr_setschedparam(&attr, &schedp);
    if (ret) {
        fprintf(stderr, "[red] pthread_attr_setschedparam\n");
        fail = 1;
    }

    if (fail == 1) {
        fprintf(stderr, "[red] create with default priority.\n");
        pattr = NULL;
    }

    ret = pthread_create(pth, pattr, func, arg);
    if (ret) {
        fprintf(stderr, "[red] pthread_create\n", ret);
        return -1;
    }

    pthread_attr_destroy(&attr);
    return 0;
}

