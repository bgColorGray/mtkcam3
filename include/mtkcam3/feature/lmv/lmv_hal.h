/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

/**
* @file lmv_hal.h
*
* LMV Hal Header File
*
*/

#ifndef _LMV_HAL_H_
#define _LMV_HAL_H_

#include <mtkcam3/feature/lmv/lmv_type.h>
#include <mtkcam/def/UITypes.h>


/**
  *@brief LMV hal class used by scenario
*/
class LMVHal
{
public:

    /**
         *@brief LMVHal constructor
       */
    LMVHal() {};

    /**
         *@brief Create LMVHal object
         *@param[in] userName : user name,i.e. who create LMV HAL object
         *@param[in] aSensorIdx : sensor index
         *@return
         *-LMVHal object
       */
    static LMVHal *CreateInstance(char const *userName, const MUINT32 &aSensorIdx);

    /**
         *@brief Destroy LMVHal object
         *@param[in] userName : user name,i.e. who destroy LMV HAL object
       */
    virtual MVOID DestroyInstance(char const *userName) = 0;

    /**
         *@brief Initialization function
         *@return
         *-LMV_RETURN_NO_ERROR indicates success, otherwise indicates fail
       */
    virtual MINT32 Init(const MUINT32 eisFactor) = 0;

    /**
         *@brief Unitialization function
         *@return
         *-LMV_RETURN_NO_ERROR indicates success, otherwise indicates fail
       */
    virtual MINT32 Uninit() = 0;

    /**
         *@brief Configure LMV
         *@details Use this API after pass1config and before pass1 start
         *@param[in] aLmvConfig : LMV config data
         *@return
         *-LMV_RETURN_NO_ERROR indicates success, otherwise indicates fail
       */
    virtual MINT32 ConfigLMV(const LMV_HAL_CONFIG_DATA &aLmvConfig) = 0;

    virtual MINT32 PrepareLMV(const LMV_HAL_ENQUEUE_DATA &aLmvEnqueData) = 0;

    /**
         *@brief Execute LMV
         *@param[in] QBufInfo : LMV result data
         *@return
         *-LMV_RETURN_NO_ERROR indicates success, otherwise indicates fail
       */
    virtual MINT32 DoLMVCalc(NSCam::NSIoPipe::NSCamIOPipe::QBufInfo const &pBufInfo, const MUINT32 rrzIdx, const MUINT32 lmvIdx) = 0;

    /**
         *@brief Get LMV algorithm result
         *@param[out] a_CMV_X_Int : LMV algo result of X-direction integer part  //Deprecated
         *@param[out] a_CMV_X_Flt  : LMV algo result of X-direction float part     //Deprecated
         *@param[out] a_CMV_Y_Int : LMV algo result of Y-direction integer part  //Deprecated
         *@param[out] a_CMV_Y_Flt  : LMV algo result of Y-direction float part     //Deprecated
         *@param[out] a_TarWidth    : LMV width crop size                                //Deprecated
         *@param[out] a_TarHeight   : LMV height crop size                               //Deprecated
         *@param[out] a_MVtoCenterX    : mv X to the left-top of center window in EIS domain
         *@param[out] a_MVtoCenterY    : mv Y to the left-top of center window in EIS domain
         *@param[out] a_isFromRRZ   : LMV input is from RRZ or not, value of  '1' means from RRZ, and sensor source must be RAW sensor
                                                                                          value of  '0' means from others, and sensor source must be YUV sensor

       */
    virtual MVOID GetLMVResult(MUINT32 &a_CMV_X_Int,
                               MUINT32 &a_CMV_X_Flt,
                               MUINT32 &a_CMV_Y_Int,
                               MUINT32 &a_CMV_Y_Flt,
                               MUINT32 &a_TarWidth,
                               MUINT32 &a_TarHeight,
                               MINT32  &a_MVtoCenterX,
                               MINT32  &a_MVtoCenterY,
                               MUINT32 &a_isFromRRZ) = 0;

    /**
         *@brief Get LMV GMV
         *@details The value is 256x
         *@param[out] aGMV_X : x-direction global motion vector between two frames
         *@param[out] aGMV_Y  : y-direction global motion vector between two frames
       */
    virtual MVOID GetGmv(MINT32 &aGMV_X, MINT32 &aGMV_Y, MUINT32 *confX=NULL, MUINT32 *confY=NULL,
        MUINT32 *MAX_GMV=NULL) = 0;

    /**
         *@brief Get LMV HW support or not
         *@param[in] aSensorIdx : sensor index
         *@return
         *-MTRUE indicates LMV HW is supported, MFALSE indicates not supported
       */
    virtual MBOOL GetLMVSupportInfo(const MUINT32 &aSensorIdx) = 0;

    virtual MINT32 GetBufLMV(android::sp<NSCam::IImageBuffer>& spBuf) = 0;
    virtual MINT32 NotifyLMV(NSCam::NSIoPipe::NSCamIOPipe::QBufInfo&  pBufInfo) = 0;
    virtual MINT32 NotifyLMV(android::sp<NSCam::IImageBuffer>& spBuf) = 0;


    /**
         *@brief Return LMV HW statistic result
         *@param[out] a_pLMV_Stat : EIS_STATISTIC_STRUCT
       */
    virtual MVOID GetLMVStatistic(EIS_STATISTIC_STRUCT *a_pLMV_Stat) = 0;


	/**
     *@brief  Get input width/height of LMV HW
   */
    virtual MVOID GetLMVInputSize(MUINT32 *aWidth, MUINT32 *aHeight) = 0;

    virtual MUINT32 GetLMVStatus() = 0;

    virtual MVOID GetRegSetting(void *data) = 0;

    virtual MVOID EnableGyroSensor() = 0;

    virtual LMVData getDefaultData(
      NSCam::NSIoPipe::NSCamIOPipe::QBufInfo const &pBufInfo, const MUINT32 rrzIdx) = 0;

protected:

    /**
         *@brief LMVHal destructor
       */
    virtual ~LMVHal() {};
};

#endif //_LMV_HAL_H_

