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
 * MediaTek Inc. (C) 2017. All rights reserved.
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

#include "gtest/gtest.h"
#include <featurePipe/core/include/IOUtil.h>
#include <featurePipe/core/include/ImageBufferPool.h>
#include "TestIO.h"

#define PIPE_MODULE_TAG "FeaturePipeTestIO"
#define PIPE_CLASS_TAG "TestIO"
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam/utils/std/DebugTimer.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_COMMON);
using namespace NSCam::Utils;

namespace NSCam{
namespace NSCamFeature{
namespace NSFeaturePipe{

#define TEST_IO_FULL            1

android::sp<IBufferPool> createIBufferPool(const char *name)
{
    return ImageBufferPool::create(name, 16, 16, eImgFmt_YV12, ImageBufferPool::USAGE_SW );
}

typedef std::list<TestIONode*> TestNodePath;

TestIONode nodeA(NODEA_NAME);
TestIONode nodeB(NODEB_NAME);
TestIONode nodeC(NODEC_NAME);
TestIONode nodeD(NODED_NAME);
TestIONode nodeE(NODEE_NAME);

void runIOTestCase(TestNodePath &recordPath, TestNodePath &displayPath, TestNodePath &nodes, TestCase *testCases, unsigned n, TestNodePath &physicalPath, TestNodePath &tinyPath)
{
    IOControl<TestIONode, TestReqInfo, TestIOUser> control;
    MUINT32 sensorID = 0; // only test sID 0

    std::vector<android::sp<IBufferPool>> fullPools;
    for(unsigned i=0;i<nodes.size();++i)
    {
        fullPools.push_back(createIBufferPool("Full"));
        fullPools[i]->allocate(1);
    }

    android::sp<IBufferPool> nodeCNextFullPool = createIBufferPool("CNextFull");
    nodeCNextFullPool->allocate(2); // B maybe has INOUT_Next
    nodeC.setInputBufferPool(nodeCNextFullPool);
    android::sp<IBufferPool> nodeCExtraPool_1 = createIBufferPool("CExtra1");
    nodeCExtraPool_1->allocate(2); // B maybe has INOUT_Next
    nodeC.setExtraBufferPool_1(nodeCExtraPool_1);
    android::sp<IBufferPool> nodeCExtraPool_2 = createIBufferPool("CExtra2");
    nodeCExtraPool_2->allocate(2); // B maybe has INOUT_Next
    nodeC.setExtraBufferPool_2(nodeCExtraPool_2);
    android::sp<IBufferPool> nodeCExtraPool_T = createIBufferPool("CExtraT");
    nodeCExtraPool_T->allocate(2); // B maybe has INOUT_Next
    nodeC.setExtraBufferPool_T(nodeCExtraPool_T);

    android::sp<IBufferPool> nodeDENextFullPool = createIBufferPool("D_E_NextFull"); //D,E use same pool
    nodeDENextFullPool->allocate(4); // maybe combine INOUT_Next
    nodeD.setInputBufferPool(nodeDENextFullPool);
    nodeE.setInputBufferPool(nodeDENextFullPool);

    control.setRoot(*displayPath.begin());

    control.addStream(STREAMTYPE_PREVIEW, displayPath);
    control.addStream(STREAMTYPE_PREVIEW_CALLBACK, displayPath);
    control.addStream(STREAMTYPE_RECORD, recordPath);
    control.addStream(STREAMTYPE_PHYSICAL, physicalPath);
    control.addStream(STREAMTYPE_TINY, tinyPath);

    for(unsigned i=0;i<n;++i)
    {
        TestCase& t = testCases[i];

        IORequest<TestIONode, TestReqInfo, TestIOUser> outputRequest;
        IOControl<TestIONode, TestReqInfo, TestIOUser>::StreamSet streams;

        if( t.hasOutput.display )
        {
            streams.insert(STREAMTYPE_PREVIEW);
        }
        if( t.hasOutput.record )
        {
            streams.insert(STREAMTYPE_RECORD);
        }
        if( t.hasOutput.phy)
        {
            streams.insert(STREAMTYPE_PHYSICAL);
        }
        if( t.hasOutput.tiny)
        {
            streams.insert(STREAMTYPE_TINY);
        }

        DebugTimer timer;
        timer.start();
        TestReqInfo reqInfo(0, t.needNode.pack(), 0, 0);

        std::map<MUINT32, IOControl<TestIONode, TestReqInfo, TestIOUser>::StreamSet> streamsMap;
        streamsMap[sensorID] = streams;
        std::unordered_map<MUINT32, IORequest<TestIONode, TestReqInfo, TestIOUser>> outReqMap;
        control.prepareMap(streamsMap, reqInfo, outReqMap);
        timer.stop();
        printf("prepare Map done cost time = %d us\n", timer.getElapsedU());

        outputRequest = outReqMap[sensorID]; // only test SID 0
        MY_LOGI("[%d] Case dump start +", i);
        control.dump(outputRequest);
        MY_LOGI("[%d] Case dump end -", i);

        std::list<TestIONode*>::iterator it, end;
        unsigned j = 0;
        for(it = nodes.begin(), end = nodes.end(); it != end; ++it)
        {
            TestIONode *node = *it;
            TestNodeExpect &nodeExpect = testCases[i].nodeExpect[j];

            printf("case(%u/%u) node(%u/%zu:%s)\n", i, n, j, nodes.size(), node->getName());

            EXPECT_EQ(nodeExpect.record,  outputRequest.needRecord(node));
            EXPECT_EQ(nodeExpect.display, outputRequest.needPreview(node));
            EXPECT_EQ(nodeExpect.extra,   outputRequest.needPreviewCallback(node));
            EXPECT_EQ(nodeExpect.full,    outputRequest.needFull(node));
            EXPECT_EQ(nodeExpect.AFull || nodeExpect.A3Full || nodeExpect.A3Full2P || nodeExpect.A3Full2P2L,
                        outputRequest.needNextFull(node));
            EXPECT_EQ(nodeExpect.phy,   outputRequest.needPhysicalOut(node));

            if(outputRequest.needNextFull(node))
            {
                IORequest<TestIONode, TestReqInfo, TestIOUser>::NextCollection nexts = outputRequest.getNextFullImg(node);
                size_t totalPath = nexts.mMap.size();
                size_t needOutNum = nexts.mList.size();
                const IORequest<TestIONode, TestReqInfo, TestIOUser>::NextBufMap &prvBufs = nexts.mMap[N_TARGET_P];
                const IORequest<TestIONode, TestReqInfo, TestIOUser>::NextBufMap &recBufs = nexts.mMap[N_TARGET_R];
                const IORequest<TestIONode, TestReqInfo, TestIOUser>::NextBufMap &tinyBufs = nexts.mMap[N_TARGET_TINY];
                EXPECT_EQ(nodeExpect.A3Full2P || nodeExpect.A3Full2P2L, (prvBufs.size() && recBufs.size()));

                if( nodeExpect.A3Full || nodeExpect.AFull )
                {
                    EXPECT_TRUE(( prvBufs.size() || recBufs.size() || tinyBufs.size() ));
                    EXPECT_TRUE( totalPath == 1 );
                }
                else if( nodeExpect.A3Full2P )
                {
                    EXPECT_TRUE((prvBufs.size() == recBufs.size()));
                    for( const auto &it : prvBufs )
                    {
                        EXPECT_TRUE((it.second.mBuffer == recBufs.at(it.first).mBuffer));
                    }
                    EXPECT_TRUE( needOutNum < (prvBufs.size() + recBufs.size()));
                }
                else if( nodeExpect.A3Full2P2L )
                {
                    EXPECT_TRUE( needOutNum == (prvBufs.size() + recBufs.size()));
                }
                for( const auto &it : prvBufs)
                {
                    EXPECT_EQ(it.second.mAttr.mAppInplace == NextAttr::APP_INPLACE_NONE, it.second.mBuffer != NULL);
                }
            }
            if(outputRequest.needFull(node))
            {
                android::sp<IIBuffer> img = fullPools[j]->requestIIBuffer();
                EXPECT_TRUE(img != NULL);
            }

            printf("case(%u/%u) node(%u/%zu:%s) end\n", i, n, j, nodes.size(), node->getName());
            j++;
        }
    }
}

#define FULL_NODES          &nodeA, &nodeB, &nodeC, &nodeD, &nodeE
#define FULL_RECORD_PATH    &nodeA, &nodeB, &nodeC, &nodeE
#define FULL_DISPLAY_PATH   &nodeA, &nodeB, &nodeC, &nodeD
#define FULL_PHYSICAL_PATH  &nodeA, &nodeC
#define FULL_TINY_PATH      &nodeA, &nodeC

TestCase testCasesFull[] = {
    //       OutputConfig                          NodeConfig                                                       NodeA Expect                                  NodeB Expect                                NodeC Expect                                NodeD Expect            NodeE Expect

    { {.display=true, .phy=true},       {.A=true},                                                            { {.display=true, .phy=true},                 {},                                         {},                                             {},                      {}             } },
    { {.display=true},                  {.A=true, .B=true},                                                 { {.full=true},                                 {.display=true},                            {},                                         {},                      {}             } },
    { {.display=true, .phy=true},       {.A=true, .C=true, .C_A3=true, .C_P=true},                           { {.A3Full=true},                                 {},                                         {.display=true, .phy=true},                            {},                      {}             } },
    { {.display=true},                  {.A=true, .B=true, .B_N=true, .C=true, .C_A=true },                  { {.AFull=true},                                 {.AFull=true},                               {.display=true},                            {},                      {}             } },
    { {.display=true, .tiny=true},      {.A=true, .B=true, .C=true, .C_A3=true, .C_T=true },               { {.full=true, .AFull=true},                      {.A3Full=true},                               {.display=true},                            {},                      {}             } },
#if 0
    { {.display=true},                  {.A=true, .A_3=true},                                               { {.display=true, .full=true},                  {},                                         {},                                         {},                      {}             } },
    { {.display=true},                  {.A=true, .A_3=true, .B=true},                                      { {.full=true},                                 {.display=true},                            {},                                         {},                      {}             } },
    { {.display=true},                  {.A=true, .A_3=true, .C=true},                                      { {.full=true},                                 {},                                         {.display=true},                            {},                      {}             } },
    { {.display=true},                  {.A=true, .A_3=true, .B=true, .C=true},                             { {.full=true},                                 {.full=true},                               {.display=true},                            {},                      {}             } },
#endif
    { {.record=true, .display=true},    {.A=true, .E=true,   .E_A=true},                                    { {.display=true, .AFull=true},                 {},                                         {},                                         {},                      {.record=true}             } },
    { {.record=true, .display=true},    {.A=true, .D=true,   .D_A=true, .E=true, .E_A=true},                { {.A3Full2P=true},                             { },                                        {},                                         {.display=true},          {.record=true}             } },
    { {.record=true, .display=true},    {.A=true, .B=true,   .B_N=true, .D=true,  .D_A=true, .E=true, .E_A=true},        { {.A3Full2P=true},                { .A3Full2P=true},                          {},                                         {.display=true},          {.record=true}             } },
    { {.record=true, .display=true},    {.A=true, .C=true,   .C_A3=true, .E=true, .E_A=true},               { {.A3Full=true},                                 {},                                       {.display=true, .AFull=true},               {},                      {.record=true}             } },
    { {.record=true, .display=true},    {.A=true, .C=true,   .C_A3=true, .D=true, .D_A=true, .E=true, .E_A=true},          { {.A3Full=true},                   {},                                      {.A3Full2P=true},                           {.display=true},          {.record=true}             } },
    { {.record=true, .display=true},    {.A=true, .C=true,   .C_A3=true, .D=true, .D_A=true, .E=true, .E_AC=true},         { {.A3Full=true},                   {},                                      {.A3Full2P2L=true},                         {.display=true},          {.record=true}             } },
#if 0
    { {.record=true, .display=true},    {.A=true, .A_3=true, .D=true, .D_Q=true},                   { {.display=true, .QFull=true},                 {},                                         {},                                         {.record=true}          {}             } },
    { {.record=true, .display=true},    {.A=true, .A_3=true, .B=true, .D=true, .D_Q=true},          { {.full=true},                                 {.display=true, .QFull=true },              {},                                         {.record=true}          {}             } },
    { {.record=true, .display=true},    {.A=true, .A_3=true, .C=true, .D=true, .D_Q=true},          { {.full=true},                                 {},                                         {.display=true, .QFull=true},               {.record=true}          {}             } },

    { {.record=true, .display=true
        ,.phy=true} ,                   {.A=true, .A_3=true, .B=true, .C=true, .D=true, .D_Q=true}, { {.full=true, .phy=true},                      {.full=true},                               {.display=true, .QFull=true},               {.record=true}          {}             } },
#endif

};

#if TEST_IO_FULL
TEST(FeatureIO, IO_FULL)
{
    TestNodePath recordPath = {FULL_RECORD_PATH}, displayPath = {FULL_DISPLAY_PATH}, nodes = {FULL_NODES};
    TestNodePath physicalPath = {FULL_PHYSICAL_PATH};
    TestNodePath tinyPath = {FULL_TINY_PATH};
    unsigned n = sizeof(testCasesFull)/sizeof(testCasesFull[0]);
    runIOTestCase(recordPath, displayPath, nodes, testCasesFull, n, physicalPath, tinyPath);
}
#endif


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
