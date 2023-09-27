/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
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
#ifndef __MSNRCORE_H__
#define __MSNRCORE_H__

// Msnr Core Lib
#include <mtkcam3/feature/msnr/IMsnr.h>


// MTKCAM
/*To-Do: remove when cam_idx_mgr.h reorder camear_custom_nvram.h order before isp_tuning_cam_info.h */
#include <camera_custom_nvram.h>
#include <mtkcam/utils/mapping_mgr/cam_idx_mgr.h>
//
#include <mtkcam/utils/imgbuf/ImageBufferHeap.h> //PortBufInfo_v1
//
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h> // tuning file naming
// MSS/MSF library
#include <mtkcam/drv/iopipe/PostProc/INormalStream.h>
#include <mtkcam/drv/iopipe/PostProc/IEgnStream.h>
#include <mtkcam/drv/def/mfbcommon_v20.h>
// Dpframework
#include <DpDataType.h>
// IHALISP
#include <mtkcam/aaa/IHalISP.h>
// STL
#include <memory>
#include <mutex>
#include <future>
#include <chrono>
#include <unordered_map>
// AOSP
#include <cutils/compiler.h>
#include <utils/RefBase.h>
// Isp profile mapper
#include <mtkcam3/feature/utils/ISPProfileMapper.h>

// NormalPipe namespace
using namespace NSCam;
using namespace NS3Av3;
using namespace android;
using namespace NSCam::NSIoPipe::NSPostProc;
using namespace NSCam::TuningUtils;
using namespace NSCam::NSIoPipe::NSEgn;
using namespace NSCam::NSIoPipe::NSMFB20;

class MsnrCore : public IMsnr
{
public:
    // Enum list
    enum OnlineDeviceMode {
        DeviceModeOff = 0,
        DeviceModeDump = 1,
        DeviceModeOn = 2,
    };
public:
    MsnrCore();

/* implementation from IMsnr */
public:
    // check input parameters and start thread
    virtual enum MsnrErr init(IMsnr::ConfigParams const& params);
    // main process
    virtual enum MsnrErr doMsnr();
    // make debug info
    virtual enum MsnrErr makeDebugInfo(IMetadata* metadata);
    // destructor
    virtual ~MsnrCore(void);
private:
    // check input parameters
    enum MsnrErr checkParams(IMsnr::ConfigParams const& params);
    // allocate mss input buffer
    enum MsnrErr allocateMssBuf();
    // deallocate mss input buffer
    enum MsnrErr deallocateMssBuf();
    // allocate msf output buffer
    enum MsnrErr allocateMsfBuf();
    // deallocate msf output buffer
    enum MsnrErr deallocateMsfBuf();
    // relese P2 debuf buffer
    enum MsnrErr deallocateDebugBuf();
    // prepare P2 tuning buffer and setISP
    enum MsnrErr prepareTunBuf();
    // prepare P2 tuning buffer and setISP for dce
    enum MsnrErr prepareDCETunBuf();
    // release P2 tuning buffer
    enum MsnrErr releaseTunBuf();
    // generate mss dumping hint for hw
    enum MsnrErr generateMssNDDHint(const IMetadata* pInHalMeta, FILE_DUMP_NAMING_HINT& hint);
    // generate msf dumping hint for hw
    enum MsnrErr generateMsfNDDHint(const IMetadata* pInHalMeta, int stage, FILE_DUMP_NAMING_HINT& hint);
    // generate p2 dumping hint for hw
    enum MsnrErr generateP2NDDHint(const IMetadata* pInHalMeta, FILE_DUMP_NAMING_HINT& hint);
    // generate MDP param
    bool generateMdpDrePQParam(DpPqParam& rMdpPqParam, int iso, int openId, const IMetadata* pMetadata, EIspProfile_T ispProfile);
    // get msf stage isp profile
    inline EIspProfile_T getIspProfile(int stage);
    inline int32_t getIspStage(int32_t iterationId);
    // common api for generating hint
    void generateNDDHintFromMetadata(FILE_DUMP_NAMING_HINT& hint, const IMetadata* pMetadata);
    // NDD dump
    void NddDump(IImageBuffer* buff, FILE_DUMP_NAMING_HINT hint, YUV_PORT type, const char *pUserString = nullptr, bool bPrintSensor = false);

private:
    // ConfigParams
    IMsnr::ConfigParams                        mConfig;
    // Normal stream
    std::unique_ptr< INormalStream, std::function<void(INormalStream*)> >
                                               mpNormalStream;
    // MSS stream
    std::unique_ptr< IEgnStream<MSSConfig>, std::function<void(IEgnStream<MSSConfig>*)> >
                                               mpMssStream;
    // HAL ISP
    std::unique_ptr< NS3Av3::IHalISP, std::function<void(NS3Av3::IHalISP*)> >
                                               mpHalIsp;
    // ISP profile mapper
    ISPProfileMapper*                          mpISPProfileMapper;
    // ISP profile mapping key
    ProfileMapperKey                           mMappingKey;
    // Mss Working buffer vector
    std::vector<sp<IImageBuffer> >             mvMssBuf;
    // Msf Working buffer vector
    std::vector<sp<IImageBuffer> >             mvMsfBuf;
    // debugbuffer
    sp<IImageBuffer>                           mDebugBuf;
    // dcesobuffer
    sp<IImageBuffer>                           mDcesoBuf;
    // Tuning buffer vector
    std::vector<std::unique_ptr<char[]> >      mvTunBuf;
    // Tuning buffer vector
    std::unique_ptr<char[]>                    mP2TunBuf;
    // tuning Param vector
    std::vector<std::unique_ptr<TuningParam> > mvTunParam;
    // tuning buffer size
    size_t                                     mRegTableSize;
    // ndd level 1 flags
    int                                        mNddDumpL1;
    // ndd level 2 flags
    int                                        mNddDumpL2;
    // online device tuning mode
    int                                        mOnlineTuning;
    // enable debug buffer for P2-MDP
    int                                        mEnableDebugBuf;
    // enable P2 dump
    int                                        mP2Dump;
    // enable CapPipe dump
    int                                        mCapPipeDump;
    // check if it is inited
    bool                                       mIsInited;

    // Thread should be the end of the class member
    // mss output buffer allocation
    std::future<enum MsnrErr>                  mMssAllocThread;
    // msf output buffer allocation
    std::future<enum MsnrErr>                  mMsfAllocThread;
    // tuning buffer and param preparation
    std::future<enum MsnrErr>                  mTunPrepareThread;
/* Attributes */
};

#endif //__MSNRCORE_H__
