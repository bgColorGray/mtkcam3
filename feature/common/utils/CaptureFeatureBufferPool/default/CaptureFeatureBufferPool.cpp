/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
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

#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/imgbuf/IIonImageBufferHeap.h>
#include <mtkcam/utils/imgbuf/IDummyImageBufferHeap.h>

#include <cutils/properties.h>
#include <utils/String8.h>

#include <algorithm>

#include "CaptureFeatureBufferPool.h"

#include <mtkcam/drv/IHalSensor.h>

// using
using NSCam::ICaptureFeatureBuffer;
using NSCam::ICaptureFeatureBufferPool;
using NSCam::CaptureFeatureBuffer;
using NSCam::CaptureFeatureBufferPool;
using NSCam::CaptureFeatureBufferPoolCore;
using NSCam::IImageBuffer;
using NSCam::IIonImageBufferHeap;
using NSCam::IDummyImageBufferHeap;
using namespace NSCam::Utils::Format;
using namespace NSCam::Utils::ULog;

// typedef
typedef std::shared_ptr<CaptureFeatureBufferPoolCore> PoolCorePtr;
typedef ICaptureFeatureBuffer::BufferInfo BufferInfoT;
typedef std::shared_ptr<CaptureFeatureBuffer> BufferPtr;
typedef std::shared_ptr<ICaptureFeatureBuffer> IBufferPtr;

// log
#define LOG_TAG "CaptureFeatureBufferPoolDefault"
CAM_ULOG_DECLARE_MODULE_ID(MOD_UTILITY);

#define MY_LOGD(fmt, arg...)        CAM_ULOGMD(fmt, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI(fmt, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME(fmt, ##arg)

// --------------------- CaptureFeatureBufferPool ----------------------- //
std::shared_ptr<ICaptureFeatureBufferPool>
ICaptureFeatureBufferPool::getInstance(char* name)
{
    return std::make_shared<CaptureFeatureBufferPool>();
}

