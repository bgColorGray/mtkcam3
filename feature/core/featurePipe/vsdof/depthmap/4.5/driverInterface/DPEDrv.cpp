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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

// Standard C header file

// Android system/core header file
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/media.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <linux/v4l2-subdev.h>
// mtkcam custom header file

// mtkcam global header file

// Module header file

// Local header file
#include "DPEDrv.h"
// Logging header
#define PIPE_CLASS_TAG "DPEDRV"
#include <PipeLog.h>
CAM_ULOG_DECLARE_MODULE_ID(MOD_SFPIPE_DEPTH_DPE);

// TODO : device name - need to discuss with drv team
#define EGN_INT_WAIT_TIMEOUT_MS (1000) //Early Poting is slow.

#define DPE_DEV_NAME "dpe"
#define MAX_PATH_SIZE 255

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

DPEV4L2Stream::DPEV4L2Stream()
{
}

DPEV4L2Stream::~DPEV4L2Stream()
{
    uninit();
}

DPEV4L2Stream*
DPEV4L2Stream::createInstance()
{
    return new DPEV4L2Stream();
}

MBOOL
DPEV4L2Stream::init()
{
    CAM_ULOGM_TAGLIFE("DPEV4L2Stream::init");
    MBOOL bRet = MTRUE;
    // open device
    // if((mVideoFd = open_device("dev/video10", 0)) >= 0)
    if(-1 != v4l2_find_video_open_dev())
    {
        // stream on
        VSDOF_LOGD("DPE Stream ON...");
        ioctl(mVideoFd, VIDIOC_STREAMON);
        // start to run deque thread
        if(mDequeThread == NULL)
        {
            mDequeThread = new DequeThread(mVideoFd);
            mDequeThread->run("DepthPipe@DPEV4L2");
        }
    }
    else
    {
        VSDOF_LOGD("ERROR : DPE Stream OFF...");
        uninit();
        bRet = MFALSE;
    }
    mInitState = bRet;
    return bRet;
}

MBOOL DPEV4L2Stream::uninit()
{
    VSDOF_LOGD("uninit +");
    if(mDequeThread != NULL)
    {
        mDequeThread->signalStop();
        mDequeThread->join();
        mDequeThread = NULL;
    }
    // Stream off
    if(mVideoFd >= 0)
    {
        MY_LOGD("stream off...");
        ioctl(mVideoFd, VIDIOC_STREAMOFF);
        MY_LOGD("close VideoFd...");
        close_device(mVideoFd);
        mVideoFd = -1;
    }
    mInitState = MFALSE;
    VSDOF_LOGD("uninit -");
    return MTRUE;
}

int
DPEV4L2Stream::
v4l2_query_cap(v4l2_capability* cap)
{
    memset(cap, 0, sizeof(*cap));

    if (ioctl(mVideoFd, VIDIOC_QUERYCAP, cap) != 0){
        MY_LOGE("vidioc_querycap fail.");
        return -1;
    }

    MY_LOGD("Driver Info:\n");
    MY_LOGD("\tDriver name   : %s\n", cap->driver);
    MY_LOGD("\tCard type     : %s\n", cap->card);
    MY_LOGD("\tBus info      : %s\n", cap->bus_info);
    MY_LOGD("\tDriver version: %d.%d.%d\n", cap->version >> 16, (cap->version >> 8) & 0xff, cap->version & 0xff);
    MY_LOGD("\tCapabilities  : 0x%08X\n", cap->capabilities);

    return 0;
}

int
DPEV4L2Stream::
v4l2_find_video_open_dev()
{
    int ret;
    char path[MAX_PATH_SIZE];
    struct v4l2_capability cap;

    for (int i = 0; i < 64; i++) {
        ret = snprintf(path, MAX_PATH_SIZE, "/dev/video%d", i);

        if (ret < 0 || ret >= MAX_PATH_SIZE) {
            continue;
        }

        mVideoFd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);

        if (mVideoFd != -1 && v4l2_query_cap(&cap) != -1) {
            if (!strncmp((const char*)cap.driver, DPE_DEV_NAME, sizeof(cap.driver))) {
                MY_LOGD("Find \'%s\' in \'%s\'\n", cap.driver, path);
                return 0;
            }
        }
        if ( mVideoFd >= 0)
        {
            close(mVideoFd);
            mVideoFd = -1;
        }
    }
err:
    return -1;
}

int DPEV4L2Stream::open_device(const char *deviceName, int nonBlock)
{
    VSDOF_LOGD("open_device : %s, non_block =%d", deviceName, nonBlock);

    struct stat st;
    if (-1 == stat(deviceName, &st)) {
        MY_LOGE("Cannot identify '%s': %d, %s", deviceName, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        MY_LOGE("\t-%s is no devicen", deviceName);
        return -1;
    }

    mVideoFd = open(deviceName, O_RDWR /* required */ | (nonBlock ? O_NONBLOCK : 0), 0);
    if (mVideoFd < 0) {
        MY_LOGE("Cannot open '%s': %d, %s", deviceName, errno, strerror(errno));
        return -1;
    }
    return mVideoFd;
}

void DPEV4L2Stream::close_device(int fd)
{
    if (-1 == close(fd))
        MY_LOGE("close error %d, %s", errno, strerror(errno));
    else
        mVideoFd = -1;
}

MBOOL DPEV4L2Stream::EGNenque(DPEParamV4L2 &enqueData)
{
    CAM_ULOGM_TAGLIFE("DPENode::EGNenque");
    // fill DPERequest
    enqueData.mDpeRequest.m_ReqNum     = 1;
    enqueData.mDpeRequest.m_pDpeConfig = &(enqueData.mDpeConfig);
    // fill v4l2_buffer
    enqueData.mV4L2Buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enqueData.mV4L2Buf.memory    = V4L2_MEMORY_USERPTR;
    enqueData.mV4L2Buf.index     = 0;
    enqueData.mV4L2Buf.m.userptr = (unsigned long)&(enqueData.mDpeRequest);
    enqueData.mV4L2Buf.length    = sizeof(enqueData.mDpeRequest);
    MBOOL bRet = MTRUE;
    if(mDequeThread != NULL && mInitState == MTRUE)
    {
        if(-1 == ioctl(mVideoFd, VIDIOC_QBUF, &(enqueData.mV4L2Buf)))
        {
            MY_LOGE("VIDIOC_QBUF(%d):%s", errno, strerror(errno));
            return MFALSE;
        }
        mDequeThread->signalDeque(enqueData);
    }
    return bRet;
}

MVOID DPEV4L2Stream::destroyInstance()
{
    delete this;
}

/*******************************************************************************
* DequeThread
********************************************************************************/

DPEV4L2Stream::DequeThread::DequeThread(int videoFd)
    : mVideoFd(videoFd)
    , mStop(MFALSE)
    , m_nfds(1)
{
}

DPEV4L2Stream::DequeThread::~DequeThread()
{
}

android::status_t DPEV4L2Stream::DequeThread::readyToRun()
{
    return android::NO_ERROR;
}

MVOID DPEV4L2Stream::DequeThread::signalDeque(const DPEParamV4L2 &enqueData)
{
    android::Mutex::Autolock lock(mThreadMutex);
    mParamQueue.push(enqueData);
    mThreadCondition.broadcast();
}

MVOID DPEV4L2Stream::DequeThread::signalStop()
{
    android::Mutex::Autolock lock(mThreadMutex);
    MY_LOGD("Signal Stop");
    mStop = MTRUE;
    mThreadCondition.broadcast();
}

bool DPEV4L2Stream::DequeThread::threadLoop()
{
    CAM_ULOGM_TAGLIFE("DPENode::DequeThread::threadLoop");
    DPEParamV4L2 param;
    while(waitParam(param))
    {
        processParam(param);
    }
    return MFALSE;
}

MBOOL DPEV4L2Stream::DequeThread::waitParam(DPEParamV4L2 &param)
{
    android::Mutex::Autolock lock(mThreadMutex);
    MBOOL bRet = MTRUE, done = MFALSE;

    while(!done)
    {
        if(mParamQueue.size())
        {
            param = mParamQueue.front();
            mParamQueue.pop();
            bRet = MTRUE;
            done = MTRUE;
        }
        else if(mStop)
        {
            bRet = MFALSE;
            done = MTRUE;
        }
        else
        {
            mThreadCondition.wait(mThreadMutex);
        }
    }

    return bRet;
}

MVOID DPEV4L2Stream::DequeThread::processParam(DPEParamV4L2 &param)
{
    MBOOL bRet = MTRUE;
    struct pollfd pollFd = {
        .fd     = mVideoFd,
        .events = POLLIN,
    };
    bRet &= poll(&pollFd, m_nfds, EGN_INT_WAIT_TIMEOUT_MS);
    if(bRet <= 0)
    {
        MY_LOGE("v4l2 poll() error : %d, %s",errno, strerror(errno));
        MY_LOGE("poll(): events=%d, timeout=%d", pollFd.revents, EGN_INT_WAIT_TIMEOUT_MS);
    }
    else
    {
        if(-1 == ioctl(this->mVideoFd, VIDIOC_DQBUF, &(param.mV4L2Buf)))
        {
            AEE_ASSERT("VIDIOC failed(%d), %s", errno, strerror(errno));
        }
        VSDOF_LOGD("DPE engine deque success");
    }

    if(param.mpfnCallback)
    {
        param.mpfnCallback(param);
    }
    else
    {
        MY_LOGE("Callback function Missed");
    }
}

}; //NSFeaturePipe_DepthMap
}; //NSCamFeature
}; //NSCam