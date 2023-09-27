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


#include <mtkcam3/feature/lmv/lmv_type.h>
#include "../lmv_drv.h"

#include <thread>
#include <chrono>

#define TEST_FRAME_NUM 100
#define TEST_SET_NUM 100

class TestLMV
{
public:
    TestLMV();

    void GenerateEnqueData();
    void DoEnque(bool forceNoDrvMagic = false);
    void SimulateP1NodeQueue(bool forceNoDrvMagic = false);
    void SimulateDriverQueue(bool forceNoDrvMagic = false);
    void End(bool forceNoDrvMagic = false);

private:
    static void Sleep();

    bool mIsEnqueDone = false;
    std::queue<LMV_ENQUEUE_INFO> mPendingFrame;
    std::queue<LMV_ENQUEUE_INFO> mDriverQueue;
    LMVDrv *mLMVDrv;
    FILE  *mFile;
};


TestLMV::TestLMV()
{
    printf("======================================\n");
    printf("           Create Test Case #1        \n");
    mLMVDrv = LMVDrv::CreateInstance(0);
    printf("======================================\n");
}

void TestLMV::Sleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 300)));
}

void TestLMV::GenerateEnqueData()
{
    printf("======================================\n");
    printf("           GenerateEnqueData          \n");
    printf("======================================\n");
    mFile = ::fopen("/data/vendor/camera_dump/lmv", "w+t");
    for( MINT32 i = 0; i < TEST_SET_NUM; i++ ) {
        for( MINT32 j = 0; j < TEST_FRAME_NUM; j++ ) {
            // simulate P1Node drop
            bool isDrop = ((std::rand() % 100 ) > 95 );
            if( !isDrop ) {
                LMV_ENQUEUE_INFO lmvEnqueInfo {
                    .magicNum = j+1,
                    .bypass   = std::rand() % 2,
                };
                mPendingFrame.push(lmvEnqueInfo);
                printf("P[#%4d(%d)]", lmvEnqueInfo.magicNum, lmvEnqueInfo.bypass);
                if( mFile != nullptr ) {
                    ::fprintf(mFile, "P[#%4d(%d)]", lmvEnqueInfo.magicNum, lmvEnqueInfo.bypass);
                }
            }
        }
    }
}

void TestLMV::DoEnque(bool forceNoDrvMagic)
{
    printf("Simalate Enqueue...\n");
    std::thread simP1NodeThread(&TestLMV::SimulateP1NodeQueue, this, forceNoDrvMagic);
    std::thread simDrvThread(&TestLMV::SimulateDriverQueue, this, forceNoDrvMagic);

    simP1NodeThread.join();
    simDrvThread.join();
}

void TestLMV::SimulateP1NodeQueue(bool forceNoDrvMagic)
{
    printf("Simalate P1Node Enqueue...\n");
    while( !mPendingFrame.empty() ) {
        Sleep();
        LMV_ENQUEUE_INFO frame = mPendingFrame.front();
        mLMVDrv->PushLMVStatus(frame);
        printf("PushToLMV[#%4d(%d)]\n", frame.magicNum, frame.bypass);

        Sleep();
        // simulate driver drop
        bool isDrop = ((std::rand() % 100 ) > 95 );
        if( !isDrop ) {
            if (forceNoDrvMagic) {
                frame.magicNum = 0;
            }
            mDriverQueue.push(frame);
            printf("PushToDriver[#%4d(%d)]\n", frame.magicNum, frame.bypass);
        }
        mPendingFrame.pop();
    }
    mIsEnqueDone = true;
    printf("All frames are dispatched to driver queue...");
}

void TestLMV::SimulateDriverQueue(bool forceNoDrvMagic)
{
    printf("Simalate Driver Enqueue...\n");
    while( !mDriverQueue.empty() || !mIsEnqueDone ) {
        Sleep();
        if( !mDriverQueue.empty() ) {
            Sleep();
            LMV_ENQUEUE_INFO current = mDriverQueue.front();
            uint32_t lmvBypass = mLMVDrv->GetLMVStatusAndUpdate(current.magicNum);
            // Compare to answer here
            if (!forceNoDrvMagic) {
                if( current.bypass != lmvBypass ) {
                    //fail
                    printf("==========ABORT==========\nResult: FAIL QQ");
                    printf("current[#%4d(%d)]lmv(%d)\n", current.magicNum, current.bypass, lmvBypass);
                    End(forceNoDrvMagic);
                }
            }
            else {
                if (lmvBypass && current.bypass != lmvBypass) {
                    // fail
                    // magic==0 should always be 0
                    printf("==========ABORT==========\nResult: FAIL QQ");
                    printf("M(0)current[#%4d(%d)]lmv(%d)\n", current.magicNum, current.bypass, lmvBypass);
                    End(forceNoDrvMagic);
                }
            }
            mDriverQueue.pop();
        }
    }
    printf("All frames are done...");
}

void TestLMV::End(bool forceNoDrvMagic)
{
    if( mFile != nullptr ) {
        ::fclose(mFile);
    }
    if (!forceNoDrvMagic) {
        if( !mPendingFrame.empty() || !mDriverQueue.empty() ) {
            printf("\n==========END==========\nResult: FAIL QQ");
            exit(1);
        }
    }

    while (!mPendingFrame.empty()) {
        mPendingFrame.pop();
    }
    while (!mDriverQueue.empty()) {
        mDriverQueue.pop();
    }
}

int main(int argc, char *argv[])
{
    printf("==========\nBEGIN TO START==========\n");
    TestLMV LMVTest;

    // 1: normal case
    LMVTest.GenerateEnqueData();
    LMVTest.DoEnque();
    LMVTest.End();

    // 2: force driver no magic
    LMVTest.GenerateEnqueData();
    LMVTest.DoEnque(true);
    LMVTest.End(true);

    printf("\n==========END==========\nResult: PASS^_^");
    return 0;
}