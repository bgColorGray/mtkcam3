/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
/*
** $Log: fd_hal_base.h $
 *
*/

#ifndef _FDVT_HAL52_H_
#define _FDVT_HAL52_H_

#include "fd_hal50_base.h"
#include <mtkcam/drv/iopipe/PostProc/IEgnStream.h>
#include <mtkcam/drv/iopipe/PostProc/IHalEgnPipe.h>
#include <mtkcam/drv/def/fdvtcommon_6s.h>

using FDVTConfig = NSCam::NSIoPipe::FDVTConfig;
using namespace NSCam::NSIoPipe::NSEgn;
using namespace NSCam;
/*******************************************************************************
*
********************************************************************************/
class halFDVT52: public halFDVT50Base
{
public:
    //
    static halFDBase* getInstance();
    virtual void destroyInstance();
    //
    /////////////////////////////////////////////////////////////////////////
    //
    // halFDBase () -
    //! \brief FD Hal constructor
    //
    /////////////////////////////////////////////////////////////////////////
    halFDVT52();

    /////////////////////////////////////////////////////////////////////////
    //
    // ~mhalCamBase () -
    //! \brief mhal cam base descontrustor
    //
    /////////////////////////////////////////////////////////////////////////
    virtual ~halFDVT52();


    /////////////////////////////////////////////////////////////////////////
    //
    // halFDGetVersion () -
    //! \brief get FD version
    //
    /////////////////////////////////////////////////////////////////////////
    virtual MINT32 halFDGetVersion();


    bool handleFDCallback(EGNParams<FDVTConfig>& rParams);


private:
    // for FD5.2
    static void* onBGThreadLoop(void*);
    virtual bool doHWFaceDetection(void *pCal_Data, void *pOption);
    virtual bool doHWAttribute(void *pCal_Data, void *pOption);

protected:
    virtual void createBGThread();
    virtual void halFDDoByVersion(void *pCal_Data, void *pOption);
    virtual struct tuning_para* getTuningParaByVersion();
    virtual MINT32 allocateAttributeBuffer();
    virtual void deallocateAttributeBuffer();
    virtual MUINT32 initFDDriver(MUINT32 fdW);
    virtual void uninitFDDriver();
    virtual MUINT8 getFDVersionForInit();

    // driver
    NSCam::NSIoPipe::FDVTINPUTDEGREE  mFDDir;
    FDVTConfig mFdvtDrvResult;
    NSCam::NSIoPipe::FDVTINPUTDEGREE              mAttDir;
    FDVTConfig          mAttDrvResult;
    NSCam::NSIoPipe::NSEgn::IEgnStream<FDVTConfig>* mpFDStream;
    // driver enque/deque
    NSCam::NSIoPipe::NSEgn::EGNParams<FDVTConfig> mFdvtParams_face;
    NSCam::NSIoPipe::NSEgn::EGNParams<FDVTConfig> mFdvtParams_attr;
    // AIE 2.0(mt6885)
    short               mgender_FM[MAX_CROP_NUM][MAX_AIE2_ATT_LEN];
    unsigned short**     mworkbufList; // left, top, right, bottom, padL, padT, padR, padB

};

#endif