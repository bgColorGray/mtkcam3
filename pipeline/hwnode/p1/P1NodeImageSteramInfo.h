
#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_HWNODE_P1_NODE_IMAGE_STERAM_INFO_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_HWNODE_P1_NODE_IMAGE_STERAM_INFO_H_

#include "P1Common.h"
#include "P1Utility.h"

namespace NSCam {
namespace v3 {
namespace NSP1Node {

using SttInfo = NSCam::plugin::streaminfo::P1STT;
using QueryData = NSCam::v3::IStreamInfo::QueryPrivateData;

class P1NodeImageStreamInfo
{

public:
    sp<IImageStreamInfo> info;
    STREAM_IMG streamImg;

    P1NodeImageStreamInfo() {
        streamImg = STREAM_UNKNOWN;
    };

public:
    const SttInfo *getSttInfo();
    const ImageBufferInfo *getSttInfoPtr();
    MSize getImgSize();
    MINT getImgFormat();
    IImageStreamInfo::BufPlanes_t const& getBufPlanes();
    ImageBufferInfo const&  getImageBufferInfo();
    MBOOL isSttExist();
public:
    char const* getStreamName()
    {
        if(info)
            return info->getStreamName();
        return 0;
    };
    StreamId_T getStreamId()
    {
        if(info)
            return info->getStreamId();
        return 0;
    };

    MBOOL isSecure()
    {
        return info->isSecure();
    };
    auto  queryPrivateData() {
        return info->queryPrivateData();
    };
}; // P1NodeImageStreamInfo

};//namespace NSP1Node
};//namespace v3
};//namespace NSCam

#endif /* _MTK_HARDWARE_INCLUDE_MTKCAM_PIPELINE_HWNODE_P1_NODE_IMAGE_STERAM_INFO_H_ */
