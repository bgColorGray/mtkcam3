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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#define LOG_TAG "PipelinePlugin/TPPureBokehProvider"

// Standard C header file
#include <stdlib.h>
#include <chrono>
#include <random>
#include <thread>
// Android system/core header file

// mtkcam custom header file

// mtkcam global header file
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
// Module header file
#include <mtkcam/drv/iopipe/SImager/IImageTransform.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>
#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
//
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
//
#include <mtkcam3/feature/stereo/hal/stereo_setting_provider.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_PUREBOKEH);
// Local header file


using namespace NSCam;
using namespace android;
using namespace std;
using namespace NSCam::NSPipelinePlugin;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)


/*******************************************************************************
* MACRO Utilities Define.
********************************************************************************/
namespace { // anonymous namespace for debug MARCO function
using AutoObject = std::unique_ptr<const char, std::function<void(const char*)>>;
//
auto
createAutoScoper(const char* funcName) -> AutoObject
{
    CAM_ULOGMD("[%s] +", funcName);
    return AutoObject(funcName, [](const char* p)
    {
        CAM_ULOGMD("[%s] -", p);
    });
}
#define SCOPED_TRACER() auto scoped_tracer = ::createAutoScoper(__FUNCTION__)
//
auto
createAutoTimer(const char* funcName, const char* text) -> AutoObject
{
    using Timing = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using DuationTime = std::chrono::duration<float, std::milli>;

    Timing startTime = std::chrono::high_resolution_clock::now();
    return AutoObject(text, [funcName, startTime](const char* p)
    {
        Timing endTime = std::chrono::high_resolution_clock::now();
        DuationTime duationTime = endTime - startTime;
        CAM_ULOGMD("[%s] %s, elapsed(ms):%.4f",funcName, p, duationTime.count());
    });
}
#define AUTO_TIMER(TEXT) auto auto_timer = ::createAutoTimer(__FUNCTION__, TEXT)
//
#define UNREFERENCED_PARAMETER(param) (param)
//
} // end anonymous namespace for debug MARCO function


/*******************************************************************************
* Alias.
********************************************************************************/
using namespace NSCam;
using namespace NSCam::NSPipelinePlugin;
using namespace NSCam::NSIoPipe::NSSImager;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Type Alias..
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using Property = FusionPlugin::Property;
using Selection = FusionPlugin::Selection;
using RequestPtr = FusionPlugin::Request::Ptr;
using RequestCallbackPtr = FusionPlugin::RequestCallback::Ptr;
//
template<typename T>
using AutoPtr             = std::unique_ptr<T, std::function<void(T*)>>;
//
using ImgPtr              = AutoPtr<IImageBuffer>;
using MetaPtr             = AutoPtr<IMetadata>;
using ImageTransformPtr   = AutoPtr<IImageTransform>;


/*******************************************************************************
* Namespace Start.
********************************************************************************/
namespace { // anonymous namespace


/*******************************************************************************
* Class Definition
********************************************************************************/
/**
 * @brief third party pure bokeh algo. provider
 */
class ThirdPartyPureBokehProvider final: public FusionPlugin::IProvider
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    ThirdPartyPureBokehProvider();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  FusionPlugin::IProvider Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:

    const Property& property() override;

    MERROR negotiate(Selection& sel) override;

    void init() override;

    MERROR process(RequestPtr requestPtr, RequestCallbackPtr callbackPtr) override;

    void abort(vector<RequestPtr>& requestPtrs) override;

    void uninit() override;

    ~ThirdPartyPureBokehProvider();
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ThirdPartyPureBokehProvider Private Operator.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    MERROR processDone(const RequestPtr& requestPtr, const RequestCallbackPtr& callbackPtr, MERROR status);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ThirdPartyPureBokehProvider Private Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    struct FlowControl
    {
        MINT8  mEnable;
        MBOOL  mIsNeedCurrentFD;
        MBOOL  mIsSupportThumbnail;
        MUINT8 mInitPhase;

        FlowControl();
    };

private:
    FlowControl mFlowControl;

};
REGISTER_PLUGIN_PROVIDER(Fusion, ThirdPartyPureBokehProvider);
/**
 * @brief utility class
 */
class TPPureBokehUtility final
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    TPPureBokehUtility() = delete;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  TPPureBokehUtility Public Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    static inline ImageTransformPtr createImageTransformPtr();

    static inline ImgPtr createImgPtr(BufferHandle::Ptr& hangle);

    static inline MetaPtr createMetaPtr(MetadataHandle::Ptr& hangle);

    static inline MVOID dump(const IImageBuffer* pImgBuf, const std::string& dumpName);

    static inline MVOID dump(IMetadata* pMetaData, const std::string& dumpName);
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  TPPureBokehUtility implementation.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ImageTransformPtr
TPPureBokehUtility::
createImageTransformPtr()
{
    return ImageTransformPtr(IImageTransform::createInstance("TPPureBokehProvider", -1, IImageTransform::OPMode_DisableWPE), [](IImageTransform *p)
    {
        p->destroyInstance();
    });
}

ImgPtr
TPPureBokehUtility::
createImgPtr(BufferHandle::Ptr& hangle)
{
    return ImgPtr(hangle->acquire(), [hangle](IImageBuffer* p)
    {
        UNREFERENCED_PARAMETER(p);
        hangle->release();
    });
};

MetaPtr
TPPureBokehUtility::
createMetaPtr(MetadataHandle::Ptr& hangle)
{
    return MetaPtr(hangle->acquire(), [hangle](IMetadata* p)
    {
        UNREFERENCED_PARAMETER(p);
        hangle->release();
    });
};

MVOID
TPPureBokehUtility::
dump(const IImageBuffer* pImgBuf, const std::string& dumpName)
{
    MY_LOGD("dump image info, dumpName:%s, info:[a:%p, si:%dx%d, st:%zu, f:0x%x, va:%p]",
        dumpName.c_str(), pImgBuf,
        pImgBuf->getImgSize().w, pImgBuf->getImgSize().h,
        pImgBuf->getBufStridesInBytes(0),
        pImgBuf->getImgFormat(),
        reinterpret_cast<void*>(pImgBuf->getBufVA(0)));
}

MVOID
TPPureBokehUtility::
dump(IMetadata* pMetaData, const std::string& dumpName)
{
    MY_LOGD("dump meta info, dumpName:%s, addr::%p, count:%u",
        dumpName.c_str(), pMetaData, pMetaData->count());
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ThirdPartyFusionProvider implementation.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ThirdPartyPureBokehProvider::FlowControl::
FlowControl()
: mEnable(-1)
, mIsNeedCurrentFD(MFALSE)
, mIsSupportThumbnail(MFALSE)
, mInitPhase(ePhase_OnPipeInit)
{
    // on:1/off:0/auto:-1
    mEnable = ::property_get_int32("vendor.debug.camera.tp.purebokeh.enable", mEnable);
    mIsNeedCurrentFD = ::property_get_bool("vendor.debug.camera.tp.purebokeh.currentfd", mIsNeedCurrentFD);
    mIsSupportThumbnail = ::property_get_bool("vendor.debug.camera.tp.purebokeh.thumbnail", mIsSupportThumbnail);
    mInitPhase = ::property_get_int32("vendor.debug.camera.tp.purebokeh.initphase", mInitPhase);
    MY_LOGD("enable:%d, needCurrentFD:%d, supportThumbnail:%d, initPhase:%u",
        mEnable, mIsNeedCurrentFD, mIsSupportThumbnail, mInitPhase);
}

ThirdPartyPureBokehProvider::
ThirdPartyPureBokehProvider()
{
    MY_LOGD("ctor:%p", this);
}


const Property&
ThirdPartyPureBokehProvider::
property()
{
    static const Property prop = [this]() -> const Property
    {
        Property ret;
        ret.mName = "TP_PUREBOKEH";
        ret.mFeatures = TP_FEATURE_PUREBOKEH;
        ret.mThumbnailTiming = (mFlowControl.mIsSupportThumbnail ? eTiming_Fusion : eTiming_P2);
        ret.mFaceData = (mFlowControl.mIsNeedCurrentFD ? eFD_Current : eFD_Cache);
        ret.mBoost = eBoost_CPU;
        ret.mInitPhase = (mFlowControl.mInitPhase == ePhase_OnPipeInit ? ePhase_OnPipeInit : ePhase_OnRequest);
        return ret;
    }();
    return prop;
}

MERROR
ThirdPartyPureBokehProvider::
negotiate(Selection& sel)
{
    SCOPED_TRACER();

    if( mFlowControl.mEnable == 0 )
    {
        MY_LOGD("force off tp pure bokeh");
        return BAD_VALUE;
    }
    // INPUT
    {
        sel.mIBufferFull
            .setRequired(MTRUE)
            .addAcceptedFormat(eImgFmt_NV21)
            .addAcceptedSize(eImgSize_Full);

        sel.mIBufferFull2
            .setRequired(MTRUE)
            .addAcceptedFormat(eImgFmt_NV21)
            .addAcceptedSize(eImgSize_Full);

        sel.mIMetadataApp.setRequired(MTRUE);
        sel.mIMetadataHal.setRequired(MTRUE);
        sel.mIMetadataHal2.setRequired(MTRUE);
        sel.mIMetadataDynamic.setRequired(MTRUE);
        sel.mIMetadataDynamic2.setRequired(MTRUE);
    }
    // OUTPUT
    {
        sel.mOBufferFull
            .setRequired(MTRUE)
            .addAcceptedFormat(eImgFmt_NV21)
            .addAcceptedSize(eImgSize_Full);

        MTK_DEPTHMAP_INFO_T depthInfo = StereoSettingProvider::getDepthmapInfo();
        sel.mOBufferDepth.setRequired(MTRUE)
            .addAcceptedFormat(depthInfo.format)
            .addAcceptedSize(eImgSize_Specified)
            .setSpecifiedSize(depthInfo.size);

        if ( mFlowControl.mIsSupportThumbnail )
        {
            sel.mOBufferThumbnail
                .setRequired(MTRUE);
        }

        sel.mOMetadataApp.setRequired(MTRUE);
        sel.mOMetadataHal.setRequired(MTRUE);
    }
    return OK;
}

void
ThirdPartyPureBokehProvider::
init()
{
    SCOPED_TRACER();
}

MERROR
ThirdPartyPureBokehProvider::
process(RequestPtr requestPtr, RequestCallbackPtr callbackPtr)
{
    SCOPED_TRACER();

    auto isValidInput = [](const RequestPtr& requestPtr) -> MBOOL
    {
        const MBOOL ret = requestPtr->mIBufferFull != nullptr
                    && requestPtr->mIBufferFull2 != nullptr
                    && requestPtr->mIMetadataApp != nullptr
                    && requestPtr->mIMetadataHal != nullptr
                    && requestPtr->mIMetadataHal2 != nullptr;
        if( !ret )
        {
            MY_LOGE("invalid request with input, req:%p, inFullImg:%p, inFullImg2:%p, inAppMeta:%p, inHalMeta:%p, inHalMeta2:%p",
                requestPtr.get(),
                requestPtr->mIBufferFull.get(),
                requestPtr->mIBufferFull2.get(),
                requestPtr->mIMetadataApp.get(),
                requestPtr->mIMetadataHal.get(),
                requestPtr->mIMetadataHal2.get());
        }
        return ret;
    };

    auto isValidOutput = [this](const RequestPtr& requestPtr) -> MBOOL
    {
        const MBOOL ret = requestPtr->mOBufferFull != nullptr
                    && requestPtr->mOBufferDepth != nullptr
                    && (mFlowControl.mIsSupportThumbnail ? (requestPtr->mOBufferThumbnail != nullptr) : MTRUE)
                    && requestPtr->mOMetadataApp != nullptr
                    && requestPtr->mOMetadataHal != nullptr;
        if( !ret )
        {
            MY_LOGE("invalid request with input, req:%p, outFullImg:%p, outDepthImg:%p, outThumbnail:%p, outAppMeta:%p, outHalMeta:%p",
                requestPtr.get(),
                requestPtr->mOBufferFull.get(),
                requestPtr->mOBufferDepth.get(),
                requestPtr->mOBufferThumbnail.get(),
                requestPtr->mOMetadataApp.get(),
                requestPtr->mOMetadataHal.get());
        }
        return ret;
    };

    MY_LOGD("process, reqAdrr:%p", requestPtr.get());

    if( !isValidInput(requestPtr) )
    {
        return processDone(requestPtr, callbackPtr, BAD_VALUE);
    }

    if( !isValidOutput(requestPtr) )
    {
        return processDone(requestPtr, callbackPtr, BAD_VALUE);
    }
    //
    //
    {
        // note: we can just call createXXXXPtr one time for a specified handle
        ImgPtr inMainImgPtr = TPPureBokehUtility::createImgPtr(requestPtr->mIBufferFull);
        ImgPtr inSubImgPtr = TPPureBokehUtility::createImgPtr(requestPtr->mIBufferFull2);
        ImgPtr outFSImgPtr = TPPureBokehUtility::createImgPtr(requestPtr->mOBufferFull);
        ImgPtr outDepthImgPtr = TPPureBokehUtility::createImgPtr(requestPtr->mOBufferDepth);
        //
        MetaPtr inAppMetaPtr = TPPureBokehUtility::createMetaPtr(requestPtr->mIMetadataApp);
        MetaPtr inMainHalMetaPtr = TPPureBokehUtility::createMetaPtr(requestPtr->mIMetadataHal);
        MetaPtr inSubHalMetaPtr = TPPureBokehUtility::createMetaPtr(requestPtr->mIMetadataHal2);
        MetaPtr outAppMetaPtr = TPPureBokehUtility::createMetaPtr(requestPtr->mOMetadataApp);
        MetaPtr outHalMetaPtr = TPPureBokehUtility::createMetaPtr(requestPtr->mOMetadataHal);
        //
        ImageTransformPtr transformPtr = TPPureBokehUtility::createImageTransformPtr();
        const MRect crop = MRect(inMainImgPtr->getImgSize().w, inMainImgPtr->getImgSize().h);
        // dump info
        {
            TPPureBokehUtility::dump(inMainImgPtr.get(), "inputMainImg");
            TPPureBokehUtility::dump(inSubImgPtr.get(), "inputSubImg");
            TPPureBokehUtility::dump(outFSImgPtr.get(), "outFSImg");
            //
            TPPureBokehUtility::dump(inAppMetaPtr.get(), "inAppMeta");
            TPPureBokehUtility::dump(inMainHalMetaPtr.get(), "inMainHalMeta");
            TPPureBokehUtility::dump(inSubHalMetaPtr.get(), "inSubHalMeta");
            TPPureBokehUtility::dump(outAppMetaPtr.get(), "outAppMeta");
            TPPureBokehUtility::dump(outHalMetaPtr.get(), "outHalMeta");
        }
        // copy input yuv to output yuv
        {
            AUTO_TIMER("proces pure bokeh algo.");
            if(transformPtr.get() != nullptr)
            {
                AUTO_TIMER("process copy(mdp) inputYuv to outputYuv");

                if( transformPtr->execute( inMainImgPtr.get(), outFSImgPtr.get(), nullptr, crop, 0, 3000) )
                {
                    MY_LOGD("success to execute image transform");
                    outFSImgPtr->syncCache(eCACHECTRL_FLUSH);
                }
                else
                {
                    MY_LOGE("failed to execute image transform");
                    return processDone(requestPtr, callbackPtr, BAD_VALUE);
                }
            }
            else
            {
                MY_LOGE("IImageTransform is nullptr, cannot generate output");
                return processDone(requestPtr, callbackPtr, BAD_VALUE);
            }
        }
        // process depth
        {
            AUTO_TIMER("process depth by copy");

            const size_t depthBufStrides = outDepthImgPtr->getBufStridesInBytes(0);
            const MSize& depthBufSize = outDepthImgPtr->getImgSize();
            const size_t maxValue = 256;
            const MFLOAT increaseFactor = maxValue/static_cast<MFLOAT>(depthBufSize.h);

            char* pDepthBufVA = reinterpret_cast<char*>(outDepthImgPtr->getBufVA(0));
            for(MINT32 i = 0; i < depthBufSize.h; i++)
            {
                const char value = maxValue - (i*increaseFactor);
                ::memset(pDepthBufVA, value, depthBufStrides);
                pDepthBufVA += depthBufStrides;
            }
            outDepthImgPtr->syncCache(eCACHECTRL_FLUSH);
        }
        // process thumbnail
        {
            if ( mFlowControl.mIsSupportThumbnail )
            {
                ImgPtr outThumbnailImgPtr = TPPureBokehUtility::createImgPtr(requestPtr->mOBufferThumbnail);

                if( (transformPtr.get() == nullptr) || (!transformPtr->execute( inMainImgPtr.get(), outThumbnailImgPtr.get(), nullptr, crop, 0, 3000)) )
                {
                    MY_LOGE("failed to generate output thumbnail, transformPtr:%p", transformPtr.get());
                    return processDone(requestPtr, callbackPtr, BAD_VALUE);
                }
                outThumbnailImgPtr->syncCache(eCACHECTRL_FLUSH);
            }
        }
    }
    //
    {
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<int> distribution(0, 500);
        const int randomTime = distribution(generator);
        const int baseTime = 1000;
        auto sleepTime = std::chrono::microseconds(baseTime + randomTime);
        MY_LOGD("sleep for %lld ms", sleepTime.count());
        {
            std::this_thread::sleep_for(sleepTime);
        }
        MY_LOGD("wait up");
    }
    return processDone(requestPtr, callbackPtr, OK);
}

MERROR
ThirdPartyPureBokehProvider::
processDone(const RequestPtr& requestPtr, const RequestCallbackPtr& callbackPtr, MERROR status)
{
    SCOPED_TRACER();

    MY_LOGD("process done, call complete, reqAddr:%p, callbackPtr:%p, status:%d",
        requestPtr.get(), callbackPtr.get(), status);

    if( callbackPtr != nullptr )
    {
        callbackPtr->onCompleted(requestPtr, status);
    }
    return OK;
}

void
ThirdPartyPureBokehProvider::
abort(vector<RequestPtr>& requestPtrs)
{
    SCOPED_TRACER();

    for(auto& item : requestPtrs)
    {
        MY_LOGD("abort request, reqAddr:%p", item.get());
    }
}

void
ThirdPartyPureBokehProvider::
uninit()
{
    SCOPED_TRACER();
}

ThirdPartyPureBokehProvider::
~ThirdPartyPureBokehProvider()
{
    MY_LOGD("dtor:%p", this);
}

}  // anonymous namespace
