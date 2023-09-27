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

#ifndef _MTK_HARDWARE_MTKCAM_HW_HANDLER_H_
#define _MTK_HARDWARE_MTKCAM_HW_HANDLER_H_

#include <vector>
#include <unordered_map>
//
#include <utils/RefBase.h>
#include <utils/Mutex.h>
//
#include <mtkcam3/pipeline/pipeline/IPipelineBufferSetFrameControl.h>
#include <mtkcam3/pipeline/pipeline/IPipelineContext.h>

using namespace std;

using IAppCallback
    = NSCam::v3::pipeline::IPipelineBufferSetFrameControl::IAppCallback;

using IDataCallback
    = NSCam::v3::pipeline::NSPipelineContext::IDataCallback;
/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace isppipeline {
namespace model {

template <typename T>
inline MBOOL
tryGetMetadata(
    const IMetadata* pMetadata,
    MUINT32 const tag,
    T & rVal
)
{
    if( pMetadata == NULL ) {
        return MFALSE;
    }
    //
    IMetadata::IEntry entry = pMetadata->entryFor(tag);
    if( !entry.isEmpty() ) {
        rVal = entry.itemAt(0, Type2Type<T>());
        return MTRUE;
    }
    return MFALSE;
}
struct RequestParams
{
    uint32_t                            requestNo = 0;
    android::wp<IAppCallback>           pAppCallback = nullptr;
    android::wp<IDataCallback>          pDataCallback = nullptr;
    IMetadata*                          pAppControl = nullptr;
    std::unordered_map<StreamId_T, android::sp<IImageStreamBuffer>> inBufs;
    std::unordered_map<StreamId_T, android::sp<IImageStreamBuffer>> outBufs;
};

struct UserTransformData
{
    StreamId_T  streamId;
    uint32_t    trandform;
};
/******************************************************************************
 *
 ******************************************************************************/
class RequestData
{
    struct BufferData
    {
        android::sp<IImageStreamBuffer>  pStreamBuffer    = nullptr;
        android::sp<IImageBufferHeap>    pBufferHeap      = nullptr;
        android::sp<IImageBuffer>        pImageBuffer     = nullptr;
    };
public:
                                        RequestData(std::string Name, struct RequestParams &param);
    virtual                             ~RequestData();
    virtual auto                        processCallback(IMetadata *pMeta = nullptr, int32_t ret = 0) -> int32_t;
    virtual auto                        lockBuffer( android::sp<IImageStreamBuffer> streamBuf, MBOOL isRead = 0) -> android::sp<IImageBuffer>;
    virtual auto                        unlockBuffer( android::sp<IImageStreamBuffer> streamBuf ) -> int32_t;
    virtual auto                        unlockAllBuffer() -> int32_t;
public:
    uint32_t                              mRequestNo;
    android::wp<IAppCallback> const       mpAppCallback;
    android::wp<IDataCallback> const      mpDataCallback;
    std::string                           mUserName;
    IMetadata                             mAppControl;
    std::unordered_map<StreamId_T, android::sp<IImageStreamBuffer>> mvInputBuffer;
    std::unordered_map<StreamId_T, android::sp<IImageStreamBuffer>> mvOutputBuffer;
    mutable android::Mutex                mDataLock;
    std::unordered_map<StreamId_T, BufferData>                       mvLockedBuffer;
};
/******************************************************************************
 *
 ******************************************************************************/
class HW_Handler : public virtual android::RefBase
{
public:
                                        HW_Handler(const char* Name);
    virtual                             ~HW_Handler();
    auto                                init( const IMetadata& param, const std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>>& vImageInfoMap ) -> int32_t;
    virtual auto                        createReqData(struct RequestParams &param) -> std::shared_ptr<RequestData>;
    virtual auto                        enqueReq( std::shared_ptr<RequestData> req ) -> int32_t = 0;
    virtual auto                        uninit() -> int32_t = 0;
protected:
    virtual auto                        initHandler( const IMetadata& param ) -> int32_t;
    auto                                getCropInfo( const IMetadata* pMetadata, MRect &rect ) -> MBOOL;
    auto                                getTransformInfo( const IMetadata* pMetadata, std::vector<UserTransformData> &transformData ) -> MBOOL;
protected:
    int64_t                             mHandlerId = 0;
    std::string                         mHandlerName;
    std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> mvInputInfo;
    std::unordered_map<StreamId_T, android::sp<IImageStreamInfo>> mvOutputInfo;
};

class HWHandlerFactory
{
public:
    static auto     createHWHandler(
                        int32_t module
                    )-> android::sp<HW_Handler>;
};
/******************************************************************************
 *
 ******************************************************************************/
};  //namespace model
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_HW_HANDLER_H_

