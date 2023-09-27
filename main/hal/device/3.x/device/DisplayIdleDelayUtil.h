/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2018. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef DISPLAY_IDLE_DELAY_UTIL_H_
#define DISPLAY_IDLE_DELAY_UTIL_H_

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include <mtkcam/def/common.h>
#include <mtkperf_resource.h>
#include <cutils/properties.h>
#include <mtkcam/utils/std/ULog.h>

using namespace android;

using perfLockAcqFunc = int (*)(int, int, int[], int);
using perfLockRelFunc = int (*)(int);

namespace NSCam {
namespace v3 {

static void loadPerfAPI(perfLockAcqFunc &lockAcq, perfLockRelFunc &lockRel)
{
    static void *libHandle = NULL;
    static void *funcA = NULL;
    static void *funcR = NULL;
    const char *perfLib = "libmtkperf_client_vendor.so";

    if (libHandle == NULL)
    {
        libHandle = dlopen(perfLib, RTLD_NOW);

        if (libHandle == NULL) {
            CAM_ULOGE(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "dlopen fail: %s\n", dlerror());
            return ;
        }
        CAM_ULOGD(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] load lib");
    }

    if (funcA == NULL)
    {
        funcA = dlsym(libHandle, "perf_lock_acq");

        if (funcA == NULL) {
            CAM_ULOGE(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "perfLockAcq error: %s\n", dlerror());
            dlclose(libHandle);
            libHandle = NULL;
            return ;
        }
        CAM_ULOGD(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] load funcA");
    }

    if (funcR == NULL)
    {
        funcR = dlsym(libHandle, "perf_lock_rel");

        if (funcR == NULL) {
            CAM_ULOGE(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "perfLockRel error: %s\n", dlerror());
            dlclose(libHandle);
            libHandle = NULL;
            funcA = NULL;
            return ;
        }
        CAM_ULOGD(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] load funcR");
    }

    lockAcq = reinterpret_cast<perfLockAcqFunc>(funcA);
    lockRel = reinterpret_cast<perfLockRelFunc>(funcR);

    return ;
}

/******************************************************************************
 *
 ******************************************************************************/
class DisplayIdleDelayUtil
{
public:
    DisplayIdleDelayUtil()
    {
        loadPerfAPI(_perfLockAcq, _perfLockRel);
    }

    virtual ~DisplayIdleDelayUtil()
    {
        disable();
    }

    virtual bool enable()
    {
        Mutex::Autolock lock(_Lock);

        if (_isDisplayIdleEnabled) {
            return true;
        }

        if (_perfLockAcq)
        {
            std::vector<int> vcmd;
            vcmd.clear();
            const int DISPLAY_IDLE_DELAY_DEFAULT = 100;
            int displayIdleDelay = ::property_get_int32("vendor.cam3dev.displayidledelay", DISPLAY_IDLE_DELAY_DEFAULT);
            if(displayIdleDelay < 0) {
                displayIdleDelay = DISPLAY_IDLE_DELAY_DEFAULT;
            }

            vcmd.push_back(PERF_RES_DISP_IDLE_TIME);
            vcmd.push_back(displayIdleDelay);

            if (vcmd.size() > 0)
            {
                _powerHalHandle = _perfLockAcq(_powerHalHandle, 0 /*timout, unit is ms*/, vcmd.data(), vcmd.size());
            }

            if (_powerHalHandle != 0)
            {
                CAM_ULOGD(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] Set display idle delay to %d, handle %d", displayIdleDelay, _powerHalHandle);
                _isDisplayIdleEnabled = true;
            }
            else
            {
                CAM_ULOGE(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] Failed to lock perf, invalid handle: %d", _powerHalHandle);
            }
        }

        return _isDisplayIdleEnabled;
    }

    virtual void disable()
    {
        Mutex::Autolock lock(_Lock);

        if (_isDisplayIdleEnabled &&
            _perfLockRel &&
            _powerHalHandle != 0)
        {
            _perfLockRel(_powerHalHandle);
            _isDisplayIdleEnabled = false;
            _powerHalHandle       = 0;
            CAM_ULOGD(NSCam::Utils::ULog::MOD_CAMERA_DEVICE, "[DisplayIdleDelayUtil] Display idle disabled");
        }
    }

protected:
    mutable Mutex           _Lock;
    bool                    _isDisplayIdleEnabled = false;
    MINT32                  _powerHalHandle       = -1;
    // perf hal
    /* function pointer to perfserv client */
    perfLockAcqFunc         _perfLockAcq = NULL;
    perfLockRelFunc         _perfLockRel = NULL;
};

/******************************************************************************
 *
 ******************************************************************************/
};
};
#endif  //DISPLAY_IDLE_DELAY_UTIL_H_
