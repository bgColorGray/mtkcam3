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

#ifndef INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMBUFFER_APPSTREAMBUFFERS_H_
#define INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMBUFFER_APPSTREAMBUFFERS_H_
//
#include <mtkcam3/main/mtkhal/core/device/3.x/types.h>
//#include <mtkcam3/pipeline/stream/IStreamBuffer.h>
//#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam3/pipeline/utils/streambuf/StreamBuffers.h>
#include <mtkcam/utils/imgbuf/IIonDevice.h>
#include <mtkcam/utils/debug/IPrinter.h>
#include <mtkcam/utils/imgbuf/IImageBuffer.h>
//
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/******************************************************************************
 *
 ******************************************************************************/
using NSCam::v3::IImageStreamInfo;
using NSCam::v3::IMetaStreamInfo;
using NSCam::v3::IImageStreamBuffer;
using NSCam::v3::IMetaStreamBuffer;
using NSCam::v3::IUsersManager;
//using NSCam::IImageBufferHeap;
using NSCam::IPrinter;
using namespace NSCam::v3;
using namespace NSCam::v3::Utils;
/******************************************************************************
 *
 ******************************************************************************/
namespace mcam {
namespace core {
namespace Utils {

/******************************************************************************
 *
 ******************************************************************************/
/**
 * An implementation of app image stream buffer.
 */
class AppImageStreamBuffer
    : public NSCam::v3::Utils::TStreamBuffer<AppImageStreamBuffer, IImageStreamBuffer>
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Definitions.


    struct  Allocator
    {
    public:     ////                            Data Members.
        android::sp<IStreamInfoT>               mpStreamInfo;

    public:     ////                            Operations.
                                                Allocator(
                                                    IStreamInfoT* pStreamInfo
                                                );
        virtual                                 ~Allocator() {}

    public:     ////                            Operator.
        virtual StreamBufferT*                  operator()(
                                                    MUINT64 bufferId,
                                                    std::shared_ptr<mcam::IImageBufferHeap> pMcamHeap,
                                                    NSCam::IImageBufferHeap* pHeap,
                                                    IStreamInfoT* pStreamInfo = NULL
                                                );
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                                Data Members.
    MUINT64 const                               mBufferId;
    std::shared_ptr<mcam::IImageBufferHeap>     mMcamImageBufferHeap;
    android::sp<NSCam::IImageBufferHeap>        mImageBufferHeap;
    MINT                                        mAcquireFence = -1;

public:     ////                                Operations.
    // notify hidl-device do not free buffer handle
    bool                                        freeBufferHandleByUser = false;

                                                AppImageStreamBuffer(
                                                    MUINT64 bufferId,
                                                    android::sp<IStreamInfoT> pStreamInfo,
                                                    std::shared_ptr<mcam::IImageBufferHeap> pMcamHeap,
                                                    android::sp<NSCam::IImageBufferHeap> pImageBufferHeap
                                                );

                                                ~AppImageStreamBuffer();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Accessors.
    MUINT64                                     getBufferId() const;
    android::sp<NSCam::IImageBufferHeap>        getImageBufferHeap() const;
    virtual MINT                                getAcquireFence()   const;
    virtual MVOID                               setAcquireFence(MINT fence);
    virtual MINT                                getReleaseFence()   const;
    virtual MVOID                               setReleaseFence(MINT fence);
public:     ////                                Attributes.
    virtual android::String8                    toString() const;

public:     ////                                Operations.
    virtual MVOID                               replaceStreamInfo(android::sp<IStreamInfoT> pNewStreamInfo);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IUsersManager Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Operations.
    virtual ssize_t                             enqueUserGraph(
                                                    android::sp<IUserGraph> pUserGraph
                                                );

};


/**
 * An implementation of app image stream buffer for error return.
 */
class AppErrorImageStreamBuffer
    : public AppImageStreamBuffer
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Definitions.

    struct  Allocator
    {
    public:     ////                            Operator.
        virtual                                 ~Allocator() {}
        virtual StreamBufferT*                  operator()(
                                                    MUINT64 bufferId,
                                                    IStreamInfoT* pStreamInfo,
                                                    MINT dupReleaseFence = -1
                                                );
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                                Data Members.
    android::sp<NSCam::Utils::Sync::IFence>     mReleaseFence;

public:     ////                                Operations.
                                                AppErrorImageStreamBuffer(
                                                    MUINT64 bufferId,
                                                    android::sp<IStreamInfoT> pStreamInfo,
                                                    MINT dupReleaseFence
                                                );

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Accessors.
    virtual MINT                                getAcquireFence()   const;
    virtual MVOID                               setAcquireFence(MINT fence);
    virtual MINT                                getReleaseFence()   const;
    virtual MVOID                               setReleaseFence(MINT fence);
};


/******************************************************************************
 *
 ******************************************************************************/
/**
 * An implementation of app metadata stream buffer.
 */
class AppMetaStreamBuffer
    : public NSCam::v3::Utils::TStreamBuffer<AppMetaStreamBuffer, IMetaStreamBuffer>
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                                Definitions.

    struct  Allocator
    {
    public:     ////                            Data Members.
        android::sp<IStreamInfoT>               mpStreamInfo;

    public:     ////                            Operations.
                                                Allocator(
                                                    IStreamInfoT* pStreamInfo
                                                );
        virtual                                 ~Allocator() {}

    public:     ////                            Operator.
        virtual StreamBufferT*                  operator()();
        virtual StreamBufferT*                  operator()(NSCam::IMetadata const& metadata);
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                                Data Members.
    NSCam::IMetadata                            mMetadata;  // IBufferT-derived object.
    MBOOL                                       mRepeating; // Non-zero indicates repeating meta settings.

public:     ////                                Operations.
                                                AppMetaStreamBuffer(
                                                    android::sp<IStreamInfoT> pStreamInfo,
                                                    IUsersManager* pUsersManager = 0
                                                );
                                                AppMetaStreamBuffer(
                                                    android::sp<IStreamInfoT> pStreamInfo,
                                                    NSCam::IMetadata const& metadata,
                                                    IUsersManager* pUsersManager = 0
                                                );

public:     ////                                Attributes.
    virtual MVOID                               setRepeating(MBOOL const repeating);
    virtual MBOOL                               isRepeating() const;
    virtual android::String8                    toString() const;

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace Utils
};      // namespace core
};      // namespace mcam
#endif  // INCLUDE_MTKCAM3_MAIN_MTKHAL_CORE_DEVICE_3_X_UTILS_STREAMBUFFER_APPSTREAMBUFFERS_H_
