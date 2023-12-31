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

#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <cutils/properties.h>

using NSCam::NSPipelinePlugin::Interceptor;
using NSCam::NSPipelinePlugin::PipelinePlugin;
using NSCam::NSPipelinePlugin::PluginRegister;
using NSCam::NSPipelinePlugin::Join;
using NSCam::NSPipelinePlugin::JoinPlugin;

using namespace NSCam::NSPipelinePlugin;
using NSCam::MSize;

using NSCam::MERROR;
using NSCam::IImageBuffer;
using NSCam::IMetadata;

#ifdef LOG_TAG
#undef LOG_TAG
#endif // LOG_TAG
#define LOG_TAG "MtkCam/TPI_S_FB"
#include <log/log.h>
#include <android/log.h>
#include <mtkcam/utils/std/ULog.h>

#define MY_LOGI(fmt, arg...)  CAM_ULOGMI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)  CAM_ULOGMD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)  CAM_ULOGMW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)  CAM_ULOGME("[%s] " fmt, __FUNCTION__, ##arg)
#define FUNCTION_IN   MY_LOGD("%s +", __FUNCTION__)
#define FUNCTION_OUT  MY_LOGD("%s -", __FUNCTION__)
CAM_ULOG_DECLARE_MODULE_ID(MOD_STREAMING_TPI_PLUGIN);

using NSCam::EImageFormat;
using NSCam::eImgFmt_NV21;
using NSCam::eImgFmt_NV12;
using NSCam::eImgFmt_YV12;
using NSCam::eImgFmt_UNKNOWN;

class S_FB_Plugin : public JoinPlugin::IProvider
{
public:
    typedef JoinPlugin::Property Property;
    typedef JoinPlugin::Selection Selection;
    typedef JoinPlugin::Request::Ptr RequestPtr;
    typedef JoinPlugin::RequestCallback::Ptr RequestCallbackPtr;
    typedef JoinPlugin::ConfigParam ConfigParam;

public:
    S_FB_Plugin();
    ~S_FB_Plugin();
    const Property& property();
    void set(MINT32 openID1, MINT32 openID2);
    void init();
    void uninit();
    void config(const ConfigParam &param);
    MERROR negotiate(Selection& sel);
    MERROR process(RequestPtr pRequest, RequestCallbackPtr pCallback);
    void abort(std::vector<RequestPtr> &pRequests);

private:
    MERROR getConfigSetting(Selection &sel);
    MERROR getP1Setting(Selection &sel);
    MERROR getP2Setting(Selection &sel);
    bool checkConfig(const IImageBuffer *in, const IImageBuffer *out) const;
    bool matchFormat(EImageFormat fmt) const;
    void copy(const IImageBuffer *in, IImageBuffer *out);
    void drawMask(IImageBuffer *buffer, float fx, float fy, float fw, float fh, char mask);

private:
    bool    mDisponly = false;
    bool    mInplace = false;
    int     mOpenID1 = 0;
    int     mOpenID2 = 0;
    bool    mUseNV21 = false;
    bool    mUseNV12 = false;
    bool    mUseYV12 = false;
    bool    mUseSize = false;
    MSize   mSize;
    int     mDemoMode = 0;
};

S_FB_Plugin::S_FB_Plugin()
{
    MY_LOGI("create S_FB plugin");
}

S_FB_Plugin::~S_FB_Plugin()
{
    MY_LOGI("destroy  S_FB plugin");
}

void S_FB_Plugin::set(MINT32 openID1, MINT32 openID2)
{
    MY_LOGD("set openID1:%d openID2:%d", openID1, openID2);
    mOpenID1 = openID1;
    mOpenID2 = openID2;
}

const S_FB_Plugin::Property& S_FB_Plugin::property()
{
    static Property prop;
    static bool inited;

    if (!inited) {
        prop.mName = "MTK FB";
        prop.mFeatures = MTK_FEATURE_FB;
        //prop.mInPlace = MTRUE;
        //prop.mFaceData = eFD_Current;
        //prop.mPosition = 0;
        inited = true;
    }
    return prop;
}

MERROR S_FB_Plugin::negotiate(Selection &sel)
{
    MERROR ret = OK;

    if( sel.mSelStage == eSelStage_CFG )
    {
        ret = getConfigSetting(sel);
    }
    else if( sel.mSelStage == eSelStage_P1 )
    {
        ret = getP1Setting(sel);
    }
    else if( sel.mSelStage == eSelStage_P2 )
    {
        ret = getP2Setting(sel);
    }

    return ret;
}

MERROR S_FB_Plugin::getConfigSetting(Selection &sel)
{
    MUINT32 sId = sel.mAllSensorIDs.size() ? sel.mAllSensorIDs[0] : (MUINT32)(-1);
    MY_LOGI("max out size(%dx%d), StreamingSize (%dx%d), AllSensorSize is %zu, first sensor is %d, Timestamp: %" PRId64,
            sel.mCfgInfo.mMaxOutSize.w, sel.mCfgInfo.mMaxOutSize.h, sel.mCfgInfo.mStreamingSize.w, sel.mCfgInfo.mStreamingSize.h, sel.mAllSensorIDs.size(), sId, sel.mTPICustomConfig.mTimestamp);

    mDisponly = property_get_bool("vendor.debug.tpi.s.fb.disponly", 0);
    mInplace = mDisponly || property_get_bool("vendor.debug.tpi.s.fb.inplace", 0);
    mUseNV21 = property_get_bool("vendor.debug.tpi.s.fb.nv21", 0);
    mUseNV12 = property_get_bool("vendor.debug.tpi.s.fb.nv12", 0);
    mUseYV12 = property_get_bool("vendor.debug.tpi.s.fb.yv12", 0);
    mUseSize = property_get_bool("vendor.debug.tpi.s.fb.size", 0);
    mSize = sel.mCfgInfo.mMaxOutSize;

    sel.mCfgOrder = 3;
    sel.mCfgJoinEntry = eJoinEntry_S_YUV;
    sel.mCfgInplace = mInplace;
    sel.mCfgCropInput = property_get_bool("vendor.debug.tpi.s.fb.crop", 0);
    sel.mCfgEnableFD = MTRUE;
    sel.mCfgEnablePerFrameNegotiate = property_get_bool("vendor.debug.tpi.s.fb.perframe", 1);
    sel.mCfgSharedInput = property_get_bool("vendor.debug.tpi.s.fb.shareinput", MFALSE);
    sel.mIBufferMain1.setRequired(MTRUE);
    if( mUseNV21 ) sel.mIBufferMain1.addAcceptedFormat(eImgFmt_NV21);
    if( mUseNV12 ) sel.mIBufferMain1.addAcceptedFormat(eImgFmt_NV12);
    if( mUseYV12 ) sel.mIBufferMain1.addAcceptedFormat(eImgFmt_YV12);
    if( mUseSize ) sel.mIBufferMain1.setSpecifiedSize(mSize);
    sel.mOBufferMain1.setRequired(MTRUE);
    //sel.mIBufferMain1.addAcceptedFormat(eImgFmt_NV21);

    IMetadata *meta = sel.mIMetadataApp.getControl().get();
    MY_LOGD("sessionMeta=%p", meta);

    // demo mode enable from APP
    IMetadata::getEntry(meta, MTK_STREAMING_FEATURE_DEMO_FB, mDemoMode);

    sel.mCfgRun = property_get_bool("vendor.debug.tpi.s.fb", (mDemoMode !=0) );

    return OK;
}

MERROR S_FB_Plugin::getP1Setting(Selection &sel)
{
    (void)sel;
    return OK;
}

MERROR S_FB_Plugin::getP2Setting(Selection &sel)
{
    IMetadata *meta = sel.mIMetadataApp.getControl().get();
    sel.mP2Run = property_get_bool("vendor.debug.tpi.s.fb.run", 1);
    MY_LOGD("mP2Run=%d meta=%p", sel.mP2Run, meta);
    return OK;
}

void S_FB_Plugin::init()
{
    MY_LOGI("init S_FB plugin");
}

void S_FB_Plugin::uninit()
{
    MY_LOGI("uninit S_FB plugin");
}

void S_FB_Plugin::config(const ConfigParam &param)
{
    MY_LOGI("config S_FB plugin: fmt=0x%x size=%dx%d stride=%d",
            param.mIBufferMain1.mFormat,
            param.mIBufferMain1.mSize.w, param.mIBufferMain1.mSize.h,
            param.mIBufferMain1.mStride);
}

void S_FB_Plugin::abort(std::vector<RequestPtr> &pRequests)
{
    (void)pRequests;
    MY_LOGD("uninit S_FB plugin");
};

MERROR S_FB_Plugin::process(RequestPtr pRequest, RequestCallbackPtr pCallback)
{
    (void)pCallback;
    MERROR ret = -EINVAL;
    MBOOL needRun = MFALSE;
    IImageBuffer *in = NULL, *out = NULL;

    needRun = MTRUE;
    MY_LOGD("enter FB_PLUGIN needRun=%d", needRun);

    if( needRun &&
        pRequest->mIBufferMain1 != NULL &&
        pRequest->mOBufferMain1 != NULL )
    {
        bool srcInplace = property_get_bool("vendor.debug.tpi.s.fb.srcInplace", 0);
        bool changeRoi = property_get_bool("vendor.debug.tpi.s.fb.changeRoi", (mDemoMode == 2) );
        MSize inSize, outSize;
        unsigned inPlane = 0, outPlane = 0;
        EImageFormat inFmt = eImgFmt_UNKNOWN, outFmt = eImgFmt_UNKNOWN;

        in = pRequest->mIBufferMain1->acquire();
        out = pRequest->mOBufferMain1->acquire();

        if( in )
        {
            inSize = in->getImgSize();
            inPlane = in->getPlaneCount();
            inFmt = (EImageFormat)in->getImgFormat();
        }
        if( out )
        {
            outSize = out->getImgSize();
            outPlane = out->getPlaneCount();
            outFmt = (EImageFormat)out->getImgFormat();
        }

        bool check = checkConfig(in, out);
        MY_LOGD("FB_PLUGIN check=%d in[%d](%dx%d)(0x%x)=%p out[%d](%dx%d)(0x%x)=%p",
                check,
                inPlane, inSize.w, inSize.h, inFmt, in,
                outPlane, outSize.w, outSize.h, outFmt, out);

        if( check && in && out )
        {
            if( !mInplace && !srcInplace )
            {
                copy(in, out);
            }
            if( srcInplace )
            {
                drawMask(in, 0.2, 0.2, 0.1, 0.2, 50);
                pRequest->mOBufferMain1_Info.mOUseSrcImageBuffer = true;
                pRequest->mOBufferMain1_Info.mOSrcImageID = eJoinImageID_MAIN1;
            }
            else
            {
                drawMask(out, 0.2, 0.2, 0.1, 0.1, 200);
            }

            if( changeRoi )
            {
                NSCam::MRectF newRoi = pRequest->mIBufferMain1_Info.mISrcZoomROI;
                newRoi.p.x = 0.0f;
                newRoi.p.y = 0.0f;
                pRequest->mOBufferMain1_Info.mODstZoomROI = newRoi;
            }

            // simulate delay
            usleep(5000);
            ret = OK;
        }

        pRequest->mIBufferMain1->release();
        pRequest->mOBufferMain1->release();
    }

    MY_LOGD("exit FB_PLUGIN ret=%d", ret);
    return ret;
}

bool S_FB_Plugin::checkConfig(const IImageBuffer *in, const IImageBuffer *out) const
{
    bool ret = false;
    if( in && out )
    {
        ret = true;
        MSize inSize = in->getImgSize();
        MSize outSize = out->getImgSize();
        EImageFormat inFmt = (EImageFormat)in->getImgFormat();
        EImageFormat outFmt = (EImageFormat)out->getImgFormat();
        if( (inFmt != outFmt) || !matchFormat(inFmt) )
        {
            MY_LOGW("FB_PLUGIN: fmt not match: useNV21(0x%x)=%d useNV12(0x%x)=%d useYV12(0x%x)=%d inFmt(0x%x) outFmt(0x%x)",
                    eImgFmt_NV21, mUseNV21,
                    eImgFmt_NV12, mUseNV12,
                    eImgFmt_YV12, mUseYV12,
                    inFmt, outFmt);
            ret = false;
        }
        if( inSize != outSize ||
            (mUseSize && inSize != mSize) )
        {
            MY_LOGW("FB_PLUGIN: size not match: useSize(%dx%d)=%d inSize=(%dx%d) outSize=(%dx%d)", mSize.w, mSize.h, mUseSize, inSize.w, inSize.h, outSize.w, outSize.h);
            ret = false;
        }

        if( mUseNV21 || mUseNV12 || mUseYV12 )
        {
            ret = ret &&
                  ((mUseNV21 && inFmt == eImgFmt_NV21) ||
                   (mUseNV12 && inFmt == eImgFmt_NV12) ||
                   (mUseYV12 && inFmt == eImgFmt_YV12));
        }
        if( mUseSize )
        {
            ret = ret && (inSize == mSize);
        }
        ret = ret && (inSize == outSize);
        ret = ret && (inFmt == outFmt);
    }
    return ret;
}

bool S_FB_Plugin::matchFormat(EImageFormat fmt) const
{
    if( mUseNV21 || mUseNV12 || mUseYV12 )
    {
        return (mUseNV21 && fmt == eImgFmt_NV21) ||
               (mUseNV12 && fmt == eImgFmt_NV12) ||
               (mUseYV12 && fmt == eImgFmt_YV12);
    }
    return true;
}

void S_FB_Plugin::copy(const IImageBuffer *in, IImageBuffer *out)
{
    if( !in || !out )
    {
        return;
    }

    unsigned inPlane = in->getPlaneCount();
    unsigned outPlane = out->getPlaneCount();

    for( unsigned i = 0; i < inPlane && i < outPlane; ++i )
    {
        char *inPtr = (char*)in->getBufVA(i);
        char *outPtr = (char*)out->getBufVA(i);
        unsigned inStride = in->getBufStridesInBytes(i);
        unsigned outStride = out->getBufStridesInBytes(i);
        unsigned inBytes = in->getBufSizeInBytes(i);
        unsigned outBytes = out->getBufSizeInBytes(i);

        if( !inPtr || !outPtr || !inStride || !outStride )
        {
            continue;
        }

        if( inStride == outStride )
        {
            memcpy(outPtr, inPtr, std::min(inBytes, outBytes));
        }
        else
        {
            unsigned stride = std::min(inStride, outStride);
            unsigned height = std::min(inBytes/inStride, outBytes/outStride);
            for( unsigned y = 0; y < height; ++y )
            {
                memcpy(outPtr+y*outStride, inPtr+y*inStride, stride);
            }
        }
    }
}

void S_FB_Plugin::drawMask(IImageBuffer *buffer, float fx, float fy, float fw, float fh, char mask)
{
    // sample: modify output buffer
    if( buffer )
    {
        char *ptr = (char*)buffer->getBufVA(0);
        if( ptr )
        {
            MSize size = buffer->getImgSize();
            int stride = buffer->getBufStridesInBytes(0);
            int y_from = fy * size.h;
            int y_to = (fy + fh) * size.h;
            int x = fx * size.w;
            int width = fw * size.w;

            y_to = std::max(0, std::min(y_to, size.h));
            y_from = std::max(0, std::min(y_from, y_to));
            x = std::max(0, std::min(x, size.w));
            width = std::max(0, std::min(width, size.w));

            for( int y = y_from; y < y_to; ++y )
            {
                memset(ptr+y*stride + x, mask, width);
            }
        }
    }
}

//REGISTER_PLUGIN_PROVIDER_DYNAMIC(Join, S_FB_Plugin, MTK_FEATURE_FB);
REGISTER_PLUGIN_PROVIDER(Join, S_FB_Plugin);
