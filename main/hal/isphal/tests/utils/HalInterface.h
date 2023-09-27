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

#pragma once
#include "types.h"
#include <vendor/mediatek/hardware/camera/isphal/1.0/ICallback.h>
#include <unordered_map>

using namespace ::vendor::mediatek::hardware::camera::isphal::V1_0;
using namespace isphal_test;
using ::android::RefBase;
using ::android::hardware::Return;
using ::android::hardware::hidl_death_recipient;
using ::android::sp;
using ::android::wp;

/**
 * Adapter for HIDL HAL interface calls; calls into the HIDL HAL interfaces.
 */
class HalInterface : virtual public RefBase
{
public:
    HalInterface();
    virtual ~HalInterface();

    const sp<hidl_death_recipient> getHidlDeathRecipient() const
    { return mClientCallback.get(); }

    const sp<ICallback> getICallback() const
    { return mClientCallback; }

    // NOTE: for performance consideration, we *move* inputBuffers and outputBuffers
    //       so that they are invalid after recordISPRequest() call.

    using FileBufferPairs = std::vector<std::pair<File, ISPBuffer>>;
    void recordISPRequest(uint32_t requestNumber,
            FileBufferPairs&& inputBuffers, FileBufferPairs&& outputBuffers);

    // timeout in milliseconds
    // TIMEOUT_NEVER may be passed to the waitResult method to indicate that it
    // should wait indefinitely for the result.
    // return ::android::OK if success, otherwise return ::android::TIMED_OUT if
    // not receiving the result in time.
    enum { TIMEOUT_NEVER = -1 };
    ::android::status_t waitResult(int timeout);

private:
    struct ClientCallback :
        virtual public ICallback,
        virtual public hidl_death_recipient
    {
        ClientCallback(const wp<HalInterface>& interface) : mInterface(interface) {}

        ::android::status_t waitResult(int timeout);

        // Implementation of android::hardware::hidl_death_recipient interface
        void serviceDied(uint64_t cookie, const wp<IBase>& who) override;

        // Implementation of vendor::mediatek::hardware::camera::isphal::V1_0::ICallback
        Return<void> processResult(const ISPResult& result) override;

        // Implementation of vendor::mediatek::hardware::camera::isphal::V1_0::ICallback
        Return<void> processEarlyResult(const ISPResult& result) override;

        struct CaptureRequest
        {
            uint32_t requestNumber;
            FileBufferPairs inputFileBufferPairs;
            FileBufferPairs outputFileBufferPairs;

            explicit CaptureRequest(uint32_t _requestNumber,
                    FileBufferPairs&& _inputBuffers,
                    FileBufferPairs&& _outputBuffers)
            {
                requestNumber = _requestNumber;
                inputFileBufferPairs = std::move(_inputBuffers);
                outputFileBufferPairs = std::move(_outputBuffers);
            }
        };
        std::unordered_map<uint32_t, CaptureRequest> mCaptureRequest;

    private:
        wp<HalInterface> mInterface;

        // mResultCV is used for the processResult() in a synchronous manner
        mutable std::mutex mResultMutex;
        std::condition_variable mResultCV;

        void writeFile(const CaptureRequest& request);
    };

    sp<ClientCallback> mClientCallback;
};
