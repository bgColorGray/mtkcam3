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

#ifndef _MTK_CAMERA_INCLUDE_DEPTHMAP_FEATURE_PIPE_DPE_DRV_H
#define _MTK_CAMERA_INCLUDE_DEPTHMAP_FEATURE_PIPE_DPE_DRV_H

// Standard C header file
#include <queue>

// Android system/core header file

// mtkcam custom header file

// mtkcam global header file
#include <camera_v4l2_dpe.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/Thread.h>
#include <linux/videodev2.h>
#include <poll.h>
// Module header file

// Local header file
#include "../DepthMapPipe_Common.h"

/*******************************************************************************
* Namespace start.
********************************************************************************/
namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe_DepthMap {

#define CHECKSUM_BUFSIZE 6
constexpr const uint32_t COOKIE_CHECKSUM1 = 0xC00C1E01;
constexpr const uint32_t COOKIE_CHECKSUM2 = 0xC00C1E02;
constexpr const uint32_t COOKIE_CHECKSUM3 = 0xC00C1E03;
constexpr const uint32_t COOKIE_CHECKSUM4 = 0xC00C1E04;
constexpr const uint32_t COOKIE_CHECKSUM5 = 0xC00C1E05;
constexpr const uint32_t COOKIE_CHECKSUM6 = 0xC00C1E06;

struct DPEConfigBuffer
{
    IImageBuffer* Dpe_InBuf_SrcImg_Y_L  = nullptr;
    IImageBuffer* Dpe_InBuf_SrcImg_Y_R  = nullptr;
    IImageBuffer* Dpe_InBuf_SrcImg_C    = nullptr;
    IImageBuffer* Dpe_InBuf_SrcImg_Y    = nullptr;
    IImageBuffer* Dpe_InBuf_ValidMap_L  = nullptr;
    IImageBuffer* Dpe_InBuf_ValidMap_R  = nullptr;
    IImageBuffer* Dpe_OutBuf_CONF       = nullptr;
    IImageBuffer* Dpe_OutBuf_OCC        = nullptr;
    IImageBuffer* Dpe_OutBuf_OCC_Ext    = nullptr;
    IImageBuffer* Dpe_InBuf_OCC         = nullptr;
    IImageBuffer* Dpe_InBuf_OCC_Ext     = nullptr;
    IImageBuffer* Dpe_OutBuf_CRM        = nullptr;
    IImageBuffer* Dpe_OutBuf_ASF_RD     = nullptr;
    IImageBuffer* Dpe_OutBuf_ASF_RD_Ext = nullptr;
    IImageBuffer* Dpe_OutBuf_ASF_HF     = nullptr;
    IImageBuffer* Dpe_OutBuf_ASF_HF_Ext = nullptr;
    IImageBuffer* Dpe_OutBuf_WMF_FILT   = nullptr;
};

class DPEParamV4L2
{
public:
    DPEParamV4L2() : __checkBuffer1{}, __checkBuffer2{}
    {
        __checksum1 = COOKIE_CHECKSUM1;
        __checksum2 = COOKIE_CHECKSUM2;
        __checksum3 = COOKIE_CHECKSUM3;
        __checksum4 = COOKIE_CHECKSUM4;
        __checksum5 = COOKIE_CHECKSUM5;
        __checksum6 = COOKIE_CHECKSUM6;
    };
    virtual ~DPEParamV4L2() {};
public:
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// Data Member
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    typedef MVOID (*PFN_CALLBACK_T)(DPEParamV4L2& rParams);

    // we do not use atomic or volatile to sepcify checksum because we want to
    // check visible side-effect problem too.
    uint32_t                    __checkBuffer1[CHECKSUM_BUFSIZE][4];
    uint32_t                    __checksum1;
    DPE_Request mDpeRequest = {};
    uint32_t                    __checksum2;
    DPE_Config mDpeConfig = {};
    uint32_t                    __checksum3;
    struct v4l2_buffer mV4L2Buf = {};
    uint32_t                    __checksum4;
    MVOID* mpCookie = NULL;
    uint32_t                    __checksum5;
    PFN_CALLBACK_T mpfnCallback = NULL;  // callback function
    // we do not use atomic or volatile to sepcify checksum because we want to
    // check visible side-effect problem too.
    uint32_t                    __checksum6;
    uint32_t                    __checkBuffer2[CHECKSUM_BUFSIZE][4];
};

inline void checkSumDump(DPEParamV4L2& c)
{
    for (int i = 0 ; i < CHECKSUM_BUFSIZE ; i++)
        CAM_ULOGI(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "%s: __checkBuffer1[%d]  %#x  %#x  %#x  %#x",
            __FUNCTION__, i, c.__checkBuffer1[i][0], c.__checkBuffer1[i][1], c.__checkBuffer1[i][2], c.__checkBuffer1[i][3]);
    //
    CAM_ULOGI(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "%s: __checksum1=%#x, __checksum2=%#x, __checksum3=%#x, __checksum4=%#x, __checksum5=%#x, __checksum6=%#x",
        __FUNCTION__, c.__checksum1, c.__checksum2, c.__checksum3, c.__checksum4, c.__checksum5, c.__checksum6);
    //
    for (int i = 0 ; i < CHECKSUM_BUFSIZE ; i++)
        CAM_ULOGI(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "%s: __checkBuffer2[%d]  %#x  %#x  %#x  %#x",
            __FUNCTION__, i, c.__checkBuffer2[i][0], c.__checkBuffer2[i][1], c.__checkBuffer2[i][2], c.__checkBuffer2[i][3]);
}

inline bool isValidCookie(DPEParamV4L2& c)
{
    if (__builtin_expect( c.__checksum1 != COOKIE_CHECKSUM1, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM1, c.__checksum1);
        return false;
    }
    if (__builtin_expect( c.__checksum2 != COOKIE_CHECKSUM2, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM2, c.__checksum2);
        return false;
    }
    if (__builtin_expect( c.__checksum3 != COOKIE_CHECKSUM3, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM3, c.__checksum3);
        return false;
    }
    if (__builtin_expect( c.__checksum4 != COOKIE_CHECKSUM4, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM4, c.__checksum4);
        return false;
    }
    if (__builtin_expect( c.__checksum5 != COOKIE_CHECKSUM5, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM5, c.__checksum5);
        return false;
    }
    if (__builtin_expect( c.__checksum6 != COOKIE_CHECKSUM6, false)) {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_SFPIPE_DEPTH, "checksum doesn't match, expected,real=%#x, %#x",
                COOKIE_CHECKSUM6, c.__checksum6);
        return false;
    }
    return true;
}


class DPEV4L2Stream
{
public:
    DPEV4L2Stream();
    virtual ~DPEV4L2Stream();
    static DPEV4L2Stream* createInstance();
    virtual MVOID destroyInstance();
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// Interface
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
    MBOOL init();
    MBOOL uninit();
    MBOOL EGNenque(DPEParamV4L2 &enqueData);
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// Internal Operaion
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    int open_device(const char *dev_name, int non_block);
    int v4l2_query_cap(v4l2_capability* cap);
    int v4l2_find_video_open_dev();
    void close_device(int fd);
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// Data Members
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    MBOOL mInitState = MFALSE;
    int mVideoFd = -1;
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// V4L2 deque thread class
// ++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    class DequeThread : public android::Thread
    {
    public:
        DequeThread(int videoFd);
        virtual ~DequeThread();
        MVOID signalDeque(const DPEParamV4L2 &enqueData);
        MVOID signalStop();
    public:
        android::status_t readyToRun();
        bool threadLoop();
    private:
        MBOOL polling();
        MBOOL waitParam(DPEParamV4L2 &param);
        MVOID processParam(DPEParamV4L2 &param);
    private:
        nfds_t m_nfds = 1;
        int mVideoFd = -1;
        MBOOL mStop = MFALSE;
        std::queue<DPEParamV4L2> mParamQueue;
        android::Mutex mThreadMutex;
        android::Condition mThreadCondition;
    };
    android::sp<DequeThread> mDequeThread = NULL;
};

}; //NSFeaturePipe_DepthMap
}; //NSCamFeature
}; //NSCam

#endif