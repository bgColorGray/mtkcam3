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

#include "RSCNode.h"

#define PIPE_CLASS_TAG "RSCNode"
#define PIPE_TRACE TRACE_RSC_NODE
#include <featurePipe/core/include/PipeLog.h>
#include <mtkcam3/feature/fsc/fsc_util.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_RSC);


#define RSC_MV_WIDTH    336
#define RSC_MV_HEIGHT   256
#define RSC_BV_WIDTH    144
#define RSC_BV_HEIGHT   256
#define RSC_SCENARIO_FPS 30

using NSCam::Utils::SensorProvider;
using NSCam::Utils::RSCMEResult;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

#define RSC_STREAM_NAME "FeaturePipe_RSC"
#define MVO_OUTPUT_WIDTH(input_width)   ((((input_width+1)/2+6)/7)*16)
#define MVO_OUTPUT_HEIGHT(input_height) (input_height>>1)
#define BVO_OUTPUT_WIDTH(input_width)   (input_width>>1)
#define BVO_OUTPUT_HEIGHT(input_height) (input_height>>1)

enum DumpMaskIndex
{
    //MASK_ALL,
    MASK_RSSO,
    MASK_PREV_RSSO,
    MASK_RSC_MV,
    MASK_RSC_BV,
    MASK_PREV_RSC_MV,
    MASK_FSC_RSSO,
    MASK_PREV_FSC_RSSO,
};

const std::vector<DumpFilter> sFilterTable =
{
    DumpFilter( MASK_RSSO,          "rsso",             MTRUE ),
    DumpFilter( MASK_PREV_RSSO,     "prev_rsso",        MTRUE ),
    DumpFilter( MASK_RSC_MV,        "rsc_mv",           MTRUE ),
    DumpFilter( MASK_RSC_BV,        "rsc_bv",           MTRUE ),
    DumpFilter( MASK_PREV_RSC_MV,   "prev_rsc_mv",      MTRUE ),
    DumpFilter( MASK_FSC_RSSO,      "fsc_rsso",         MTRUE ),
    DumpFilter( MASK_PREV_FSC_RSSO, "prev_fsc_rsso",    MTRUE ),
};

RSCNode::RSCNode(const char *name)
    : CamNodeULogHandler(Utils::ULog::MOD_STREAMING_RSC)
    , StreamingFeatureNode(name)
    , mRSCStream(NULL)
{
    TRACE_FUNC_ENTER();
    this->addWaitQueue(&mRequests);
    TRACE_FUNC_EXIT();
}

RSCNode::~RSCNode()
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC_EXIT();
}

MBOOL RSCNode::onData(DataID id, const RequestPtr &data)
{
    TRACE_FUNC_ENTER();
    TRACE_FUNC("Frame %d: %s arrived", data->mRequestNo, ID2Name(id));
    MBOOL ret = MFALSE;
    if( id == ID_ROOT_TO_RSC )
    {
        mRequests.enque(data);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL RSCNode::onInit()
{
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "RSC:onInit");
    TRACE_FUNC_ENTER();
    StreamingFeatureNode::onInit();
    enableDumpMask(sFilterTable);

    MBOOL ret = MFALSE;
    MY_LOGI("create & init RSCStream ++");
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "RSC:init stream");
    // enable after IT
    mRSCStream = NSCam::NSIoPipe::NSRsc::IRscStream::createInstance(RSC_STREAM_NAME);

    if( !mRSCStream )
    {
        MY_LOGE("Failed to create rsc module");
        return ret;
    }

    bool enableRSCTuning = getPropertyValue(KEY_FORCE_RSC_TUNING, VAL_FORCE_RSC_TUNING);
    if( enableRSCTuning )
    {
        mRSCStream = RSCTuningStream::createInstance(mRSCStream);
    }

    if( mRSCStream )
    {
        ret = mRSCStream->init();
    }

    P2_CAM_TRACE_END(TRACE_ADVANCED);
    MY_LOGI("create & init RSCStream --, needFSCResize(%d)", mPipeUsage.supportEnlargeRsso());

    // Use 128 bits(16 bytes) to store 7 blocks motions
    // MIN_MVO_STRIDE = (((width+1)/2+6)/7)*16
    //            336 = (((288  +1)/2+6)/7)*16
    mMVBufferPool = ImageBufferPool::create("rsc_mv", MSize(RSC_MV_WIDTH,RSC_MV_HEIGHT), eImgFmt_STA_BYTE, ImageBufferPool::USAGE_HW);
    mBVBufferPool = ImageBufferPool::create("rsc_bv", MSize(RSC_BV_WIDTH,RSC_BV_HEIGHT), eImgFmt_STA_BYTE, ImageBufferPool::USAGE_HW);
    mDummyBufferPool = ImageBufferPool::create("rsc_dummy", MSize(288,512), eImgFmt_STA_BYTE, ImageBufferPool::USAGE_HW);
    if( needFSCResize() )
    {
        mFSCRSSOBufferPool = ImageBufferPool::create("fsc_rsso", MSize(288,512), eImgFmt_STA_BYTE, ImageBufferPool::USAGE_HW_AND_SW);
    }

    if (mPipeUsage.supportAISS() && mpSensorProvider == nullptr)
    {
        if ((mpSensorProvider = SensorProvider::createInstance(PIPE_CLASS_TAG)) != NULL)
        {
            if (mpSensorProvider->initRSCME(RSC_MV_WIDTH * RSC_MV_HEIGHT,
               RSC_BV_WIDTH * RSC_BV_HEIGHT) != MTRUE)
            {
                MY_LOGW("SensorProvider initRSCME  failed");
            }
        }
        else
        {
            MY_LOGW("SensorProvider create failed");
        }
    }
    TRACE_FUNC_EXIT();
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    return ret;
}

MBOOL RSCNode::onUninit()
{
    TRACE_FUNC_ENTER();
    if( mRSCStream )
    {
        mRSCStream->uninit();
        mRSCStream->destroyInstance(RSC_STREAM_NAME);
        mRSCStream = NULL;
    }
    mLastMV = NULL;
    mDummy = NULL;
    ImageBufferPool::destroy(mMVBufferPool);
    ImageBufferPool::destroy(mBVBufferPool);
    ImageBufferPool::destroy(mDummyBufferPool);
    mPrevFSCinRSCImgPair.mFSCRsso = NULL;
    mCurrFSCinRSCImgPair.mFSCRsso = NULL;
    ImageBufferPool::destroy(mFSCRSSOBufferPool);

    if (mpSensorProvider != NULL)
    {
        MY_LOGD("mpSensorProvider uninit");
        mpSensorProvider = NULL;
    }
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL RSCNode::onThreadStart()
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MTRUE;
    Timer timer(MTRUE);
    if( mMVBufferPool != NULL && mBVBufferPool != NULL && mDummyBufferPool != NULL)
    {
        mMVBufferPool->allocate(4);
        mBVBufferPool->allocate(3);
        mDummyBufferPool->allocate(1);
        timer.stop();
        MY_LOGD("RSC %s buf in %d ms", STR_ALLOCATE, timer.getElapsed());
    }
    if( needFSCResize() )
    {
        if( mFSCRSSOBufferPool != NULL)
        {
            mFSCRSSOBufferPool->allocate(10);
        }
        else
        {
            MY_LOGE("FSC no buffer pool!");
            ret = MFALSE;
        }
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MBOOL RSCNode::onThreadStop()
{
    TRACE_FUNC_ENTER();
    this->waitRSCStreamBaseDone();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL RSCNode::onThreadLoop()
{
    TRACE_FUNC("Waitloop");
    RequestPtr request;
    if( !waitAllQueue() )
    {
        return MFALSE;
    }
    if( !mRequests.deque(request) )
    {
        MY_LOGE("Request deque out of sync");
        return MFALSE;
    }
    if( request == NULL )
    {
        MY_LOGE("Request out of sync");
        return MFALSE;
    }

    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_REQ(TRACE_DEFAULT, request);
    request->mTimer.startRSC();
    markNodeEnter(request);
    TRACE_FUNC("Frame %d in RSC needRSC %d", request->mRequestNo, request->needRSC());
    if( needFSCResize() )
    {
        mCurrFSCinRSCImgPair.mCropRectF = request->getVar<MRectF>(SFP_VAR::FSC_RSSO_CROP_REGION, MRectF(0, 0));
    }

    if( request->needRSC() )
    {
        processRSC(request);
    }
    else
    {
        handleResultData(request, RSCResult());
    }

    if( needFSCResize() )
    {
        mPrevFSCinRSCImgPair = mCurrFSCinRSCImgPair;
    }
    request->mTimer.stopRSC();
    TRACE_FUNC_EXIT();
    return MTRUE;
}

MBOOL RSCNode::processRSC(const RequestPtr &request)
{
    TRACE_FUNC_ENTER();

    RSCEnqueData data;
    prepareRSCEnqueData(request, data);

    if (data.useDummy)
    {
        //return result immediately for first frame to speed up pipeline
        handleResultData(request, RSCResult());
    }
    else
    {
        NSIoPipe::RSCConfig rscconfig;
        prepareRSCConfig(data, rscconfig);

        RSCParam param;
        param.mRSCConfigVec.push_back(rscconfig);

        enqueRSCStream(param, data);
    }

    TRACE_FUNC_EXIT();
    return MTRUE;
}

MVOID RSCNode::prepareRSCEnqueData(const RequestPtr &request, RSCEnqueData &data)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);
    TRACE_FUNC_ENTER();

    IImageBuffer *prevRsso = NULL, *currRsso = NULL;

    prevRsso = request->getVar<IImageBuffer*>(SFP_VAR::PREV_RSSO, NULL);
    currRsso = request->getVar<IImageBuffer*>(SFP_VAR::CURR_RSSO, NULL);

    if ( needFSCResize() )
    {
        data.mPrevFSCOrgRsso = prevRsso;
        data.mCurrFSCOrgRsso = currRsso;
        prepareFSCRsso(prevRsso, mPrevFSCinRSCImgPair);
        data.mPrevFSCRsso = mPrevFSCinRSCImgPair.mFSCRsso;//keep reference
        prepareFSCRsso(currRsso, mCurrFSCinRSCImgPair);
        data.mCurrFSCRsso = mCurrFSCinRSCImgPair.mFSCRsso;//keep reference

        MY_LOGD_IF(FSCUtil::getFSCDebugLevel(mPipeUsage.getFSCMode()), "sensor(%d) Frame %d rsso(prev/curr)=(%p,%p) fsc_rsso(prev/curr)=(%p,%p)", mSensorIndex, request->mRequestNo, prevRsso, currRsso,
            data.mPrevFSCRsso.get(), data.mCurrFSCRsso.get());

        if (mPrevFSCinRSCImgPair.mFSCRsso != NULL)
            prevRsso = mPrevFSCinRSCImgPair.mFSCRsso->getImageBufferPtr();
        if (mCurrFSCinRSCImgPair.mFSCRsso != NULL)
            currRsso = mCurrFSCinRSCImgPair.mFSCRsso->getImageBufferPtr();
    }

    if ( prevRsso != NULL && currRsso != NULL && prevRsso->getImgSize() == currRsso->getImgSize() )
    {
        data.mPrevRsso = prevRsso;
        data.mCurrRsso = currRsso;
        data.useDummy  = MFALSE;
    }
    else
    {
        MSize prevRssoSize = prevRsso ? prevRsso->getImgSize() : MSize();
        MSize currRssoSize = currRsso ? currRsso->getImgSize() : MSize();

        MY_LOGW("sensor(%d) Frame %d rsso(prev/curr)=(%p_%dx%d,%p_%dx%d)", mSensorIndex, request->mRequestNo, prevRsso, prevRssoSize.w, prevRssoSize.h,
            currRsso, currRssoSize.w, currRssoSize.h);

        if( mDummy == NULL )
        {
            mDummy = mDummyBufferPool->requestIIBuffer();
            mDummy->getImageBufferPtr()->setExtParam(MSize(288, 163));
        }

        data.mPrevRsso = mDummy->getImageBufferPtr();
        data.mCurrRsso = mDummy->getImageBufferPtr();
        data.useDummy = MTRUE;
    }

    MSize rssoSize = data.mCurrRsso->getImgSize();

    data.mRequest = request;
    data.mMV = mMVBufferPool->requestIIBuffer();
    data.mMV->getImageBufferPtr()->setExtParam(MSize(MVO_OUTPUT_WIDTH(rssoSize.w), MVO_OUTPUT_HEIGHT(rssoSize.h)));
    data.mBV = mBVBufferPool->requestIIBuffer();
    data.mBV->getImageBufferPtr()->setExtParam(MSize(BVO_OUTPUT_WIDTH(rssoSize.w), BVO_OUTPUT_HEIGHT(rssoSize.h)));

    if( data.useDummy )
    {
        mLastMV = NULL;
    }
    else
    {
        data.mPrevMV = mLastMV;
        mLastMV = data.mMV;
    }

    if( data.mPrevMV == NULL )
    {
        data.mPrevMV = mMVBufferPool->requestIIBuffer();
        data.mPrevMV ->getImageBufferPtr()->setExtParam(MSize(MVO_OUTPUT_WIDTH(rssoSize.w), MVO_OUTPUT_HEIGHT(rssoSize.h)));
    }

    TRACE_FUNC_EXIT();
}

MBOOL RSCNode::prepareFSCRsso(IImageBuffer *iRsso, FSCinRSCImgPair &fscinRSCImgPair)
{
    if (iRsso == NULL)
    {
        fscinRSCImgPair.mFSCRsso = NULL;
    }
    else
    {
        if (fscinRSCImgPair.mRssoTimestamp != iRsso->getTimestamp() || fscinRSCImgPair.mFSCRsso == NULL)
        {
            fscinRSCImgPair.mRssoTimestamp = iRsso->getTimestamp();// store image pair
            //MDP+copy last line
            MSize sourceSize = iRsso->getImgSize();
            if (sourceSize.h > 0)
                sourceSize.h -= 1;/*last line is pattern*/
            iRsso->syncCache(eCACHECTRL_INVALID);

            //get new buffer
            fscinRSCImgPair.mFSCRsso = mFSCRSSOBufferPool->requestIIBuffer();
            IImageBuffer *fscRsso = fscinRSCImgPair.mFSCRsso->getImageBufferPtr();
            //keep aspect ratio
            MSize targetSize = fscRsso->getImgSize();
            targetSize.h = targetSize.w * sourceSize.h / sourceSize.w;
            fscRsso->setExtParam(MSize(targetSize.w, targetSize.h));

            //prepare output
            MDPWrapper::P2IO_OUTPUT_ARRAY outputs;
            P2IO out;
            out.mBuffer = fscRsso;
            out.mDMAConstrain = mPipeUsage.getDMAConstrain();
            if ((fscinRSCImgPair.mCropRectF.s.w != 0) && (fscinRSCImgPair.mCropRectF.s.h != 0))
            {
                out.mCropRect = fscinRSCImgPair.mCropRectF;
            }
            else
            {
                out.mCropRect = MRectF(sourceSize.w, sourceSize.h);
            }
            out.mCropDstSize = targetSize;
            outputs.push_back(out);

            if (!mMDP.process(RSC_SCENARIO_FPS, iRsso, outputs))
            {
                MY_LOGE("FSC RSSO MDP failed!");
            }

            // copy last line
            MUINT8 *pSrc = (MUINT8*)iRsso->getBufVA(0);
            pSrc += (iRsso->getBufStridesInBytes(0)*sourceSize.h);
            MUINT8 *pDst = (MUINT8*)fscRsso->getBufVA(0);
            pDst += (fscRsso->getBufStridesInBytes(0)*targetSize.h);
            if( sourceSize.h > 1 &&  targetSize.h > 1 )
            {
                const int RSS_APLI_DATA_BYTES = 4;
                memcpy(pDst, pSrc, RSS_APLI_DATA_BYTES);
                fscRsso->setExtParam(MSize(targetSize.w, targetSize.h+1/*last line is pattern*/));
            }
            else
            {
                MY_LOGW("Invalid FSC RSSO sourceSize(%dx%d),targetSize(%dx%d)",
                        sourceSize.w, sourceSize.h, targetSize.w, targetSize.h);
            }

            MY_LOGD_IF(FSCUtil::getFSCDebugLevel(mPipeUsage.getFSCMode()), "rsso(%p) t(%" PRIi64 ") %dx%d_%zu->%dx%d_%zu"
                " (%f,%f,%f,%f) fsc_rsso(%p) copy(0x%x)",
                iRsso, iRsso->getTimestamp(), iRsso->getImgSize().w, iRsso->getImgSize().h, iRsso->getBufStridesInBytes(0),
                fscRsso->getImgSize().w, fscRsso->getImgSize().h, fscRsso->getBufStridesInBytes(0),
                fscinRSCImgPair.mCropRectF.p.x, fscinRSCImgPair.mCropRectF.p.y, fscinRSCImgPair.mCropRectF.s.w, fscinRSCImgPair.mCropRectF.s.h,
                fscRsso, *(MUINT32*)pSrc);
            fscRsso->syncCache(eCACHECTRL_FLUSH);

        }
    }
    return MTRUE;
}

MVOID RSCNode::prepareRSCConfig(RSCEnqueData &data, NSIoPipe::RSCConfig &config)
{
    TRACE_FUNC_ENTER();

    IImageBuffer2RSCBufInfo(NSIoPipe::DMA_RSC_IMGI_C, data.mCurrRsso, config.Rsc_Imgi_c);
    IImageBuffer2RSCBufInfo(NSIoPipe::DMA_RSC_IMGI_P, data.mPrevRsso, config.Rsc_Imgi_p);
    IImageBuffer2RSCBufInfo(NSIoPipe::DMA_RSC_MVO, data.mMV->getImageBufferPtr(), config.Rsc_mvo);
    IImageBuffer2RSCBufInfo(NSIoPipe::DMA_RSC_BVO, data.mBV->getImageBufferPtr(), config.Rsc_bvo);
    if( data.mPrevMV != NULL )
    {
        IImageBuffer2RSCBufInfo(NSIoPipe::DMA_RSC_MVI, data.mPrevMV->getImageBufferPtr(), config.Rsc_mvi);
        config.Rsc_Ctrl_Skip_Pre_Mv = false;
    }
    else
    {
        config.Rsc_Ctrl_Skip_Pre_Mv = true;
    }

    if( data.mCurrRsso )
    {
        config.Rsc_Size_Height = data.mCurrRsso->getImgSize().h;
        config.Rsc_Size_Width = data.mCurrRsso->getImgSize().w;
        config.Rsc_Ctrl_Init_MV_Waddr     = ((config.Rsc_Size_Width+1)/2-1)/7;
        config.Rsc_Ctrl_Init_MV_Flush_cnt = ((config.Rsc_Size_Width+1)/2-1)%7;
    }

    if( data.mPrevRsso )
    {
        config.Rsc_Size_Height_p = data.mPrevRsso->getImgSize().h;
        config.Rsc_Size_Width_p = data.mPrevRsso->getImgSize().w;
    }

    TRACE_FUNC("Rsc_Ctrl_Skip_Pre_Mv(%d) Rsc_Size_Width(%d) Rsc_Size_Height(%d) Rsc_Ctrl_Init_MV_Waddr(%d) Rsc_Ctrl_Init_MV_Flush_cnt(%d)",
            config.Rsc_Ctrl_Skip_Pre_Mv, config.Rsc_Size_Width, config.Rsc_Size_Height, config.Rsc_Ctrl_Init_MV_Waddr, config.Rsc_Ctrl_Init_MV_Flush_cnt);
    TRACE_FUNC("Rsc_Size_Width_p(%d) Rsc_Size_Height_p(%d)",
            config.Rsc_Size_Width_p, config.Rsc_Size_Height_p);

    TRACE_FUNC_EXIT();
}

MVOID RSCNode::enqueRSCStream(const RSCParam &param, const RSCEnqueData &data)
{
    P2_CAM_TRACE_ASYNC_BEGIN(TRACE_ADVANCED, "RSC:enqueRSCStream", data.mRequest->mRequestNo);
    TRACE_FUNC_ENTER();

    MY_LOGD("sensor(%d) Frame %d rsc enque start", mSensorIndex, data.mRequest->mRequestNo);
    data.setWatchDog(makeWatchDog(data.mRequest, DISPATCH_KEY_RSC));
    data.mRequest->mTimer.startEnqueRSC();
    this->incExtThreadDependency();
    this->enqueRSCStreamBase(mRSCStream, param, data);

    TRACE_FUNC_EXIT();
}

MVOID RSCNode::onRSCStreamBaseCB(const RSCParam &param, const RSCEnqueData &data)
{
    // This function is not thread safe,
    // avoid accessing RSCNode class memebers
    TRACE_FUNC_ENTER();
    (void)(param);

    data.setWatchDog(nullptr);
    RequestPtr request = data.mRequest;
    if( request == NULL )
    {
        MY_LOGE("Missing request");
    }
    else
    {
        request->mTimer.stopEnqueRSC();

        MY_LOGD("sensor(%d) Frame %d rsc enque done in %d ms, useDummy(%d)", mSensorIndex, request->mRequestNo, request->mTimer.getElapsedEnqueRSC(), data.useDummy);

        if( request->needDump() )
        {
            if( data.mCurrRsso != NULL && allowDump(MASK_RSSO) )
            {
                data.mCurrRsso->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mCurrRsso, "rsso");
            }
            if( data.mPrevRsso != NULL && allowDump(MASK_PREV_RSSO) )
            {
                data.mPrevRsso->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mPrevRsso, "prev_rsso");
            }
            if( data.mMV != NULL && allowDump(MASK_RSC_MV) )
            {
                data.mMV->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mMV->getImageBufferPtr(), "rsc_mv");
            }
            if( data.mBV != NULL && allowDump(MASK_RSC_BV) )
            {
                data.mBV->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mBV->getImageBufferPtr(), "rsc_bv");
            }
            if( data.mPrevMV != NULL && allowDump(MASK_PREV_RSC_MV) )
            {
                data.mPrevMV->getImageBuffer()->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mPrevMV->getImageBufferPtr(), "rsc_prev_mv");
            }
            if( data.mPrevFSCOrgRsso != NULL && allowDump(MASK_PREV_FSC_RSSO) )
            {
                data.mPrevFSCOrgRsso->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mPrevFSCOrgRsso, "fsc_prev_rsso");
            }
            if( data.mCurrFSCOrgRsso != NULL && allowDump(MASK_FSC_RSSO) )
            {
                data.mCurrFSCOrgRsso->syncCache(eCACHECTRL_INVALID);
                dumpData(data.mRequest, data.mCurrFSCOrgRsso, "fsc_rsso");
            }
        }

        if( data.useDummy )
        {
            handleResultData(request, RSCResult());
            // After handleResultData, RSSO maybe not available. DONT use RSSO after handleResultData().
        }
        else
        {

            const NSIoPipe::RSCConfig *rscconfig = param.mRSCConfigVec.data();
            RSCResult::RSC_STA_0 rscSta;
            rscSta.value = (rscconfig != NULL) ? rscconfig->feedback.RSC_STA_0 : 0;

            copyRSCInfoToSensorProvider(request, data, rscconfig);
            handleResultData(request, RSCResult(data.mMV, data.mBV, (data.mCurrRsso != NULL) ?
                             data.mCurrRsso->getImgSize() : MSize(0, 0), rscSta, true));
            // After handleResultData, RSSO maybe not available. DONT use RSSO after handleResultData().
        }

        P2_CAM_TRACE_ASYNC_END(TRACE_ADVANCED, "RSC:enqueRSCStream", request->mRequestNo);
        request->mTimer.stopRSC();
    }

    this->decExtThreadDependency();
    TRACE_FUNC_EXIT();
}

MVOID RSCNode::handleResultData(const RequestPtr &request, const RSCResult &result)
{
    P2_CAM_TRACE_CALL(TRACE_ADVANCED);

    markNodeExit(request);
    if( mPipeUsage.supportEISRSC() )
    {
        handleData(ID_RSC_TO_EIS, RSCData(result, request));
    }
    if( mPipeUsage.support3DNRRSC() && mPipeUsage.supportP2AP2() )
    {
        handleData(ID_RSC_TO_P2G, RSCData(result, request));
    }
    handleData(ID_RSC_TO_HELPER, HelperData(FeaturePipeParam::MSG_RSSO_DONE, request));
}

MVOID RSCNode::IImageBuffer2RSCBufInfo(NSIoPipe::RSCDMAPort dmaPort, IImageBuffer *pImageBuffer, NSIoPipe::RSCBufInfo& rscBufInfo)
{
    TRACE_FUNC_ENTER();

    if( pImageBuffer )
    {
        rscBufInfo.dmaport = dmaPort;
        rscBufInfo.memID = pImageBuffer->getFD(0);                      //  memory ID
        rscBufInfo.u4BufVA = pImageBuffer->getBufVA(0);                 //  Vir Address of pool
        rscBufInfo.u4BufPA = pImageBuffer->getBufPA(0);                 //  Phy Address of pool
        rscBufInfo.u4BufSize = pImageBuffer->getBufSizeInBytes(0);      //  Per buffer size
        rscBufInfo.u4Stride = pImageBuffer->getBufStridesInBytes(0);    //  Buffer Stride
        rscBufInfo.fd = pImageBuffer->getPlaneFD(0);
        rscBufInfo.offset = pImageBuffer->getPlaneOffsetInBytes(0);

        TRACE_FUNC("dmaPort(%d) u4BufVA(%p) u4BufSize(%d)", rscBufInfo.dmaport, rscBufInfo.u4BufVA, rscBufInfo.u4BufSize);
        TRACE_FUNC("memID(%d) u4BufPA(%p) u4Stride(%d)", rscBufInfo.memID, rscBufInfo.u4BufPA, rscBufInfo.u4Stride);
        TRACE_FUNC("fd(%d) offset(%d)", rscBufInfo.fd, rscBufInfo.offset);
    }

    TRACE_FUNC_EXIT();
}

MBOOL RSCNode::needFSCResize()
{
    return mPipeUsage.supportEnlargeRsso();
}


MVOID RSCNode::copyRSCInfoToSensorProvider(
    const RequestPtr &request, const RSCEnqueData &data,
    const NSIoPipe::RSCConfig *rscconfig)
{
    TRACE_FUNC_ENTER();
    P2_CAM_TRACE_BEGIN(TRACE_ADVANCED, "RSC:copyRSCInfo");
    if (mpSensorProvider != NULL && data.useDummy != MTRUE)
    {
        RSCMEResult rscRes;
        rscRes.requestNo = request->mRequestNo;
        if (data.mMV != NULL && data.mBV != NULL)
        {
            rscRes.isValid = MTRUE;

            IImageBuffer *pImgBufMV = data.mMV->getImageBufferPtr();
            IImageBuffer *pImgBufBV = data.mBV->getImageBufferPtr();

            if (pImgBufMV) pImgBufMV->syncCache(eCACHECTRL_INVALID);
            if (pImgBufBV) pImgBufBV->syncCache(eCACHECTRL_INVALID);

            rscRes.sensorID = request->mMasterID;
            rscRes.pMV = (MUINT8*) ((pImgBufMV != NULL) ? pImgBufMV->getBufVA(0): NULL);
            rscRes.pBV = (MUINT8*) ((pImgBufBV != NULL) ? pImgBufBV->getBufVA(0): NULL);
            rscRes.TS = (data.mCurrRsso != NULL) ? data.mCurrRsso->getTimestamp(): 0;
            rscRes.rrzoSize = request->getSrcCropInfo(request->getMasterID()).mRRZOSize;
            rscRes.rssoSize = (data.mCurrRsso != NULL) ?
                data.mCurrRsso->getImgSize() : MSize(0, 0);

            rscRes.mvSize = (pImgBufMV != NULL) ? pImgBufMV->getImgSize().size() : 0;
            rscRes.bvSize = (pImgBufBV != NULL) ? pImgBufBV->getImgSize().size() : 0;
            rscRes.staGMV = (rscconfig != NULL) ? rscconfig->feedback.RSC_STA_0 : 0;

            mpSensorProvider->pushRSCME(rscRes);
        }
    }
    P2_CAM_TRACE_END(TRACE_ADVANCED);
    TRACE_FUNC_EXIT();
    return;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
