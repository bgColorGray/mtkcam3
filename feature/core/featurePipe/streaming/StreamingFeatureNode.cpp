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

#include "StreamingFeatureNode.h"

#define PIPE_CLASS_TAG "Node"
#define PIPE_TRACE TRACE_STREAMING_FEATURE_NODE
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);

using namespace NSCam::NSIoPipe;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
const char* StreamingFeatureDataHandler::ID2Name(DataID id)
{
#define MAKE_NAME_CASE(name)  \
    case name: return #name;

    switch(id)
    {
    case ID_ROOT_ENQUE:                     return "root_enque";
    case ID_ROOT_TO_P2G:                    return "root_to_p2g";
    case ID_ROOT_TO_RSC:                    return "root_to_rsc";
    case ID_ROOT_TO_DEPTH:                  return "root_to_depth";
    case ID_ROOT_TO_TOF:                    return "root_to_tof";
    case ID_ROOT_TO_EIS:                    return "root_to_eis";
    case ID_ROOT_TO_TRACKING:               return "root_to_tracking";
    case ID_P2G_TO_P2SW:                    return "p2g_to_p2sw";
    case ID_P2G_TO_PMSS:                    return "p2g_to_pmss";
    case ID_P2G_TO_MSS:                     return "p2g_to_mss";
    case ID_P2G_TO_MSF:                     return "p2g_to_msf";
    case ID_P2G_TO_WARP_P:                  return "p2g_to_warp_p";
    case ID_P2G_TO_WARP_R:                  return "p2g_to_warp_r";
    case ID_P2G_TO_EIS_P2DONE:              return "p2g_to_eis_done";
    case ID_P2G_TO_VNR:                     return "p2g_to_vnr";
    case ID_P2G_TO_PMDP:                    return "p2g_to_pmdp";
    case ID_P2G_TO_HELPER:                  return "p2g_to_helper";
    case ID_P2G_TO_BOKEH:                   return "p2g_to_bokeh";
    case ID_P2G_TO_XNODE:                   return "p2g_to_xnode";
    case ID_VNR_TO_NEXT_FULLIMG:            return "vnr_to_next_fullimg";
    case ID_VNR_TO_NEXT_P:                  return "vnr_to_next_p";
    case ID_VNR_TO_NEXT_R:                  return "vnr_to_next_r";
    case ID_DEPTH_TO_HELPER:                return "depth_to_helper";
    case ID_BOKEH_TO_HELPER:                return "bokeh_to_helper";
    case ID_WARP_TO_HELPER:                 return "warp_to_helper";
    case ID_ASYNC_TO_HELPER:                return "async_to_helper";
    case ID_HELPER_TO_ASYNC:                return "helper_to_async";
    case ID_MSS_TO_MSF:                     return "mss_to_msf";
    case ID_PMSS_TO_MSF:                    return "pmss_to_msf";
    case ID_P2SW_TO_MSF:                    return "p2sw_to_msf";
    case ID_EIS_TO_WARP_P:                  return "eis_to_warp_p";
    case ID_EIS_TO_WARP_R:                  return "eis_to_warp_r";
    case ID_P2G_TO_VENDOR_FULLIMG:          return "p2g_to_vendor";
    case ID_BOKEH_TO_VENDOR_FULLIMG:        return "bokeh_to_vendor";
    case ID_BOKEH_TO_WARP_P_FULLIMG:        return "bokeh_to_warp_p";
    case ID_BOKEH_TO_WARP_R_FULLIMG:        return "bokeh_to_warp_r";
    case ID_BOKEH_TO_XNODE:                 return "bokeh_to_xnode";
    case ID_TPI_TO_NEXT:                    return "tpi_to_next";
    case ID_TPI_TO_NEXT_P:                  return "tpi_to_next_p";
    case ID_TPI_TO_NEXT_R:                  return "tpi_to_next_r";
    case ID_VMDP_TO_NEXT:                   return "vmdp_to_next";
    case ID_VMDP_TO_NEXT_P:                 return "vmdp_to_next_p";
    case ID_VMDP_TO_NEXT_R:                 return "vmdp_to_next_r";
    case ID_VMDP_TO_XNODE:                  return "vmdp_to_xnode";
    case ID_VMDP_TO_HELPER:                 return "vmdp_to_helper";
    case ID_RSC_TO_HELPER:                  return "rsc_to_helper";
    case ID_RSC_TO_EIS:                     return "rsc_to_eis";
    case ID_XNODE_TO_HELPER:                return "xnode_to_helper";
    case ID_DEPTH_TO_BOKEH:                 return "depth_to_bokeh";
    case ID_DEPTH_P2_TO_BOKEH:              return "depth_p2_to_bokeh";
    case ID_DEPTH_TO_VENDOR:                return "depth_to_vendor";
    case ID_DEPTH_P2_TO_VENDOR:             return "depth_p2_to_vendor";
    case ID_TOF_TO_NEXT:                    return "tof_to_next";
    case ID_RSC_TO_P2G:                     return "rsc_to_p2g";
    case ID_TRACKING_TO_HELPER:             return "tracking_to_helper";
    default:                                return "unknown";
    };
#undef MAKE_NAME_CASE
}

NodeSignal::NodeSignal()
    : mSignal(0)
    , mStatus(0)
{
    mLastEnqueTime = Timer::getTimeSpec();
}

NodeSignal::~NodeSignal()
{
}

MVOID NodeSignal::setSignal(Signal signal)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mSignal |= signal;
    mCondition.notify_all();
}

MVOID NodeSignal::clearSignal(Signal signal)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mSignal &= ~signal;
}

MBOOL NodeSignal::getSignal(Signal signal)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return (mSignal & signal);
}

MVOID NodeSignal::waitSignal(Signal signal)
{
    std::unique_lock<std::mutex> lock(mMutex);
    while( !(mSignal & signal) )
    {
        mCondition.wait(lock);
    }
}

MVOID NodeSignal::setStatus(Status status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mStatus |= status;
}

MVOID NodeSignal::clearStatus(Status status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mStatus &= ~status;
}

MBOOL NodeSignal::getStatus(Status status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return (mStatus & status);
}

MUINT32 NodeSignal::getIdleTimeMS()
{
    std::lock_guard<std::mutex> lock(mMutex);
    timespec now = Timer::getTimeSpec();
    MUINT32 diffMs = Timer::diff(mLastEnqueTime, now);
    return diffMs;
}

MVOID NodeSignal::notifyEnque()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mLastEnqueTime = Timer::getTimeSpec();
    mStatus &= ~NodeSignal::STATUS_IN_FLUSH;
}

StreamingFeatureDataHandler::~StreamingFeatureDataHandler()
{
}

StreamingFeatureNode::StreamingFeatureNode(const char *name)
    : CamThreadNode(name)
    , mSensorIndex(-1)
    , mNodeDebugLV(0)
    , mPipeUsage()
    , mDebugScanLine(NULL)
{
}

StreamingFeatureNode::~StreamingFeatureNode()
{
    if( mDebugScanLine )
    {
        mDebugScanLine->destroyInstance();
        mDebugScanLine = NULL;
    }
}

MBOOL StreamingFeatureNode::onInit()
{
    mNodeDebugLV = getFormattedPropertyValue("debug.%s", this->getName());
    mUsePerFrameSetting = property_get_int32(KEY_USE_PER_FRAME_SETTING, VAL_USE_PER_FRAME_SETTING);
    return MTRUE;
}

MVOID StreamingFeatureNode::setSensorIndex(MUINT32 sensorIndex)
{
    mSensorIndex = sensorIndex;
}

MVOID StreamingFeatureNode::setPipeUsage(const StreamingFeaturePipeUsage &usage)
{
    mPipeUsage = usage;
}

MVOID StreamingFeatureNode::setNodeSignal(const android::sp<NodeSignal> &nodeSignal)
{
    mNodeSignal = nodeSignal;
}

MBOOL StreamingFeatureNode::dumpNddData(const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const BasicImg &img, const TuningUtils::YUV_PORT &port, const char *str)
{
    return img.mBuffer != NULL ? dumpNddData(hint, img.mBuffer->getImageBufferPtr(), port, str) : MFALSE;
}

MBOOL StreamingFeatureNode::dumpNddData(const TuningUtils::FILE_DUMP_NAMING_HINT &hint, const ImgBuffer &img, const TuningUtils::YUV_PORT &port, const char *str)
{
    return img != NULL ? dumpNddData(hint, img->getImageBufferPtr(), port, str) : MFALSE;
}

MBOOL StreamingFeatureNode::dumpNddData(TuningUtils::FILE_DUMP_NAMING_HINT hint, IImageBuffer *buffer, const TuningUtils::YUV_PORT &portIndex, const char *pUserStr)
{
    if( buffer )
    {
        char fileName[256] = {0};
        extract(&hint, buffer);
        genFileName_YUV(fileName, sizeof(fileName), &hint, portIndex, pUserStr);

        MY_LOGD("dump to: %s", fileName);
        buffer->saveToFile(fileName);
    }
    return MTRUE;
}

NextIO::Policy StreamingFeatureNode::getIOPolicy(const NextIO::ReqInfo&) const
{
    return NextIO::Policy();
}

MVOID StreamingFeatureNode::drawScanLine(IImageBuffer *buffer)
{
    if( mDebugScanLine == NULL)
    {
        mDebugScanLine = DebugScanLine::createInstance();
    }

    if( mDebugScanLine )
    {
        mDebugScanLine->drawScanLine(buffer->getImgSize().w, buffer->getImgSize().h, (void*)(buffer->getBufVA(0)), buffer->getBufSizeInBytes(0), buffer->getBufStridesInBytes(0));
    }
}

MVOID StreamingFeatureNode::enableDumpMask(const std::vector<DumpFilter> &vFilter, const char *postFix)
{
    std::string prop = "vendor.debug.mask.";
    const char *name = this->getName();
    const char *post = (postFix != NULL) ? postFix : (name != NULL) ? name : "";
    prop += std::string(post);
    mDebugDumpMask = 0;
    MINT32 tmp = getPropertyValue(prop.c_str(), -1);
    MBOOL print = (tmp != -1);

    for(const DumpFilter &filter : vFilter)
    {
        mDebugDumpMask |= filter.defaultDump ? (1 << filter.index) : 0;
        if( print ) MY_LOGI("%s %d=%s", name, (1 << filter.index), filter.name);
    }

    if(tmp != -1)
    {
        mDebugDumpMask = tmp;
    }
    MY_LOGI("%s current dumpMask(0x%x) prop(%s)", name, mDebugDumpMask, prop.c_str());
}

MBOOL StreamingFeatureNode::allowDump(MUINT8 maskIndex) const
{
    return (mDebugDumpMask & (1 << maskIndex));
}

MBOOL StreamingFeatureNode::dumpNamedData(const RequestPtr &request, IImageBuffer *buffer, const PRINTF_ARGS_FN &argsFn)
{
    TRACE_FUNC_ENTER();
    MBOOL ret = MFALSE;
    if( buffer )
    {
        char name[256] = {0};
        MY_LOGE_IF( (mySnprintf(name, sizeof(name), argsFn) < 0), "dump buf name snprintf FAIL");
        MUINT32 stride, pbpp, ibpp, width, height, size;
        MINT format = buffer->getImgFormat();
        stride = buffer->getBufStridesInBytes(0);
        pbpp = buffer->getPlaneBitsPerPixel(0);
        ibpp = buffer->getImgBitsPerPixel();
        size = buffer->getBufSizeInBytes(0);
        pbpp = pbpp ? pbpp : 8;
        width = stride * 8 / pbpp;
        width = width ? width : 1;
        ibpp = ibpp ? ibpp : 8;
        height = size / width;
        if( buffer->getPlaneCount() == 1 )
        {
          height = height * 8 / ibpp;
        }

        char path[256];
        int res = snprintf(path, sizeof(path), "/data/vendor/dump/%04d_r%04d_%s_%dx%d_%dx%d.%s.bin",
                request->mRequestNo, request->mRecordNo, name,
                buffer->getImgSize().w, buffer->getImgSize().h, width, height, Fmt2Name(format));
        MY_LOGE_IF( (res < 0), "dump buf path snprintf FAIL");
        TRACE_FUNC("dump to %s", path);
        buffer->saveToFile(path);
        ret = MTRUE;
    }
    TRACE_FUNC_EXIT();
    return ret;
}

MUINT32 StreamingFeatureNode::dumpData(const char *buffer, MUINT32 size, const char *filename)
{
    uint32_t writeCount = 0;
    int fd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if( fd < 0 )
    {
        MY_LOGE("Cannot create file [%s]", filename);
    }
    else
    {
        for( int cnt = 0, nw = 0; writeCount < size; ++cnt )
        {
            nw = ::write(fd, buffer + writeCount, size - writeCount);
            if( nw < 0 )
            {
                MY_LOGE("Cannot write to file [%s]", filename);
                break;
            }
            writeCount += nw;
        }
        ::close(fd);
    }
    return writeCount;
}

MBOOL StreamingFeatureNode::loadData(IImageBuffer *buffer, const char *filename)
{
    MBOOL ret = MFALSE;
    if( buffer )
    {
        loadData((char*)buffer->getBufVA(0), 0, filename);
        ret = MTRUE;
    }
    return MFALSE;
}

MUINT32 StreamingFeatureNode::loadData(char *buffer, size_t size, const char *filename)
{
    uint32_t readCount = 0;
    int fd = ::open(filename, O_RDONLY);
    if( fd < 0 )
    {
        MY_LOGE("Cannot open file [%s]", filename);
    }
    else
    {
        if( size == 0 )
        {
            off_t readSize = ::lseek(fd, 0, SEEK_END);
            size = (readSize < 0) ? 0 : readSize;
            ::lseek(fd, 0, SEEK_SET);
        }
        for( int cnt = 0, nr = 0; readCount < size; ++cnt )
        {
            nr = ::read(fd, buffer + readCount, size - readCount);
            if( nr < 0 )
            {
                MY_LOGE("Cannot read from file [%s]", filename);
                break;
            }
            readCount += nr;
        }
        ::close(fd);
    }
    return readCount;
}

MVOID StreamingFeatureNode::markNodeEnter(const RequestPtr &request) const
{
    CAM_ULOG_ENTER(this, Utils::ULog::REQ_STR_FPIPE_REQUEST, request->mRequestNo);
}

MVOID StreamingFeatureNode::markNodeExit(const RequestPtr &request) const
{
    CAM_ULOG_EXIT(this, Utils::ULog::REQ_STR_FPIPE_REQUEST, request->mRequestNo);
}

Utils::ULog::ULogTimeBombHandle StreamingFeatureNode::makeWatchDog(const RequestPtr &request, const char* dispatchKey) const
{
    const int WARNMS = 1000, MAXABORTBOUND = 80000;
    Utils::ULog::ULogTimeBombHandle ret = nullptr;
#if SUPPORT_WATCHDOG_TIME == 40
    const int ABORTMS = 40000;
#else
    const int ABORTMS = 10000;
#endif
    ret = CAM_ULOG_TIMEBOMB_VERBOSE_CREATE(this, dispatchKey, WARNMS, ABORTMS,
               MAXABORTBOUND, "Frame %d has not dequed from driver", request->mRequestNo);
    return ret;
}

MBOOL SFN_TYPE::isValid() const
{
    return ( mType == SFN_TYPE::UNI || mType == SFN_TYPE::DIV || mType == SFN_TYPE::TWIN_P || mType == SFN_TYPE::TWIN_R );
}

MBOOL SFN_TYPE::isUni() const
{
    return ( mType == SFN_TYPE::UNI );
}

MBOOL SFN_TYPE::isDiv() const
{
    return ( mType == SFN_TYPE::DIV );
}

MBOOL SFN_TYPE::isTwinP() const
{
    return ( mType == SFN_TYPE::TWIN_P );
}

MBOOL SFN_TYPE::isTwinR() const
{
    return ( mType == SFN_TYPE::TWIN_R );
}

MBOOL SFN_TYPE::isPreview() const
{
    return ( mType == SFN_TYPE::TWIN_P || mType == SFN_TYPE::UNI );
}

MBOOL SFN_TYPE::isRecord() const
{
    return ( mType == SFN_TYPE::TWIN_R || mType == SFN_TYPE::UNI );
}

const char* SFN_TYPE::Type2Name() const
{
    switch(mType)
    {
    case SFN_TYPE::UNI:      return "SFN::UNI";
    case SFN_TYPE::DIV:      return "SFN::DIV";
    case SFN_TYPE::TWIN_P:   return "SFN::TWIN_P";
    case SFN_TYPE::TWIN_R:   return "SFN::TWIN_R";
    case SFN_TYPE::BAD:
    default:                 return "SFN::BAD";
    };
}

const char* SFN_TYPE::getDumpPostfix() const
{
    switch(mType)
    {
    case SFN_TYPE::UNI:      return "uni";
    case SFN_TYPE::DIV:      return "div";
    case SFN_TYPE::TWIN_P:   return "disp";
    case SFN_TYPE::TWIN_R:   return "rec";
    case SFN_TYPE::BAD:
    default:                 return "bad";
    };
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
