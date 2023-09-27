#define LOG_TAG "MfllCore/Dump"

#include "MfllFeatureDump.h"
#include <mtkcam3/feature/mfnr/MfllLog.h>
#include "MfllCore.h"

using namespace mfll;
using namespace std;
using std::vector;
using std::string;

#ifndef MFLL_PYRAMID_SIZE
#define MFLL_PYRAMID_SIZE MFLL_MAX_PYRAMID_SIZE
#endif

static const char* __get_ext_name(IMfllImageBuffer* pImg)
{
    switch (pImg->getImageFormat()){
        case ImageFormat_Yuy2:      return "yuy2";
        case ImageFormat_Raw12:     return "raw12";
        case ImageFormat_Raw10:     return "raw";
        case ImageFormat_Raw8:      return "raw8";
        case ImageFormat_Yv16:      return "yv16";
        case ImageFormat_I422:      return "i422";
        case ImageFormat_I420:      return "i420";
        case ImageFormat_Y8:        return "y";
        case ImageFormat_Nv21:      return "nv21";
        case ImageFormat_Nv12:      return "nv12";
        case ImageFormat_YUV_P010:  return "packed_word";
        case ImageFormat_YUV_P012:  return "packed_word";
        case ImageFormat_YUV_P012_UP:   return "packed_word";
        case ImageFormat_Nv16:      return "nv16";
        case ImageFormat_Sta32:     return "sta32";
        default:break;
    }
    return "unknown";
}

/* if extName is nullptr, this method looks up the extension name from __get_ext_name */
static MfllErr __dump_image(
        IMfllImageBuffer* pImg,
        const char* /*prefix*/,
        const char* stageName,
        const char* bufferName,
        const char* extName,
        const MfllMiddlewareInfo_t& info,
        bool commonDump)
{
    if (pImg == nullptr) {
        return MfllErr_NullPointer;
    }

    // prefix, folder + file pattern
    string _f = MFLL_DUMP_DEFAUL_PATH;
    bool isYUV10 = pImg->getImageFormat() == ImageFormat_YUV_P010;
    bool isYUV12 = (pImg->getImageFormat() == ImageFormat_YUV_P012) || (pImg->getImageFormat() == ImageFormat_YUV_P012_UP);
    string bayerOrder = (isYUV10||isYUV12)?"s0":to_string(info.bayerOrder);
    int iBits = info.rawBitNum;

    if (isYUV10) iBits = 10;
    if (isYUV12) iBits = 12;

    if (isYUV10 || isYUV12)
        _f += string(MFLL_DUMP_FILE_NAME_RAW_ALIGNMENT);
    else
        _f += string(MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT);

    auto rp = [&_f](string token, string value)
    {
        auto pos = _f.find(token);
        if ( pos != std::string::npos ) {
            _f.replace(pos, token.length(), value);
        }
    };

    char strbuf[256] = {0};

    // width, height
    rp(MFLL_DUMP_PATTERN_WIDTH,  to_string(pImg->getWidth()));
    rp(MFLL_DUMP_PATTERN_HEIGHT, to_string(pImg->getHeight()));

    // iso, shutterUs
    rp(MFLL_DUMP_PATTERN_ISO, to_string(info.iso));
    rp(MFLL_DUMP_PATTERN_EXP, to_string(info.shutterUs));

    // uniqueKey
    int n = ::sprintf(strbuf, "%09d", info.uniqueKey);
    if (n < 0) {
        mfllLogW("sprintf failed writint uniqueKey, strbuf:%s", strbuf);
    }
    rp(MFLL_DUMP_PATTERN_UNIQUEKEY, strbuf);

    // frame numbuer
    n = ::sprintf(strbuf, "%04d", info.frameKey);
    if (n < 0) {
       mfllLogW("sprintf failed writing frameKey, strbuf:%s", strbuf);
    }
    rp(MFLL_DUMP_PATTERN_FRAMENUM, strbuf);

    // request number
    n = ::sprintf(strbuf, "%04d", info.requestKey);
    if (n < 0) {
       mfllLogW("sprintf failed writing requestKey, strbuf:%s", strbuf);
    }
    rp(MFLL_DUMP_PATTERN_REQUESTNUM, strbuf);

    // stage name, buffer name, extension name
    rp(MFLL_DUMP_PATTERN_STAGE, stageName ? stageName : "");
    rp(MFLL_DUMP_PATTERN_BUFFERNAME, bufferName ? bufferName : "");
    rp(MFLL_DUMP_PATTERN_EXTENSION, extName == nullptr ? __get_ext_name(pImg) : extName);

    //PW-PH-BW
    rp(MFLL_DUMP_PATTERN_ALIGNWIDTH, to_string(pImg->getAlignedWidth()));
    rp(MFLL_DUMP_PATTERN_ALIGNHEIGHT, to_string(pImg->getAlignedHeight()));
    rp(MFLL_DUMP_PATTERN_STRIDE, to_string(pImg->getBufStridesInBytes(0)));

    // bayer order, raw bit num, sensor id
    mfllLogD("bayerOrder=%d, bit=%d", info.bayerOrder, iBits);
    rp(MFLL_DUMP_PATTERN_BAYERORDER, bayerOrder);
    rp(MFLL_DUMP_PATTERN_BITS, to_string(iBits));

    mfllLogD("save image %s, commonDump(%d)", _f.c_str(), commonDump);

    return pImg->saveFile(_f.c_str(), commonDump);
}


static std::vector<enum EventType> EVENTS_TO_LISTEN_INITIALIZER(void)
{
    std::vector<enum EventType> v;
    #define LISTEN(x) v.push_back(x)
    /* release RAW buffers if necessary */
    LISTEN(EventType_CaptureRaw);
    LISTEN(EventType_CaptureRrzo);
    LISTEN(EventType_CaptureYuvQ);
    LISTEN(EventType_CaptureMss);
    LISTEN(EventType_Blending);

    /* release ME/MC related buffers */
    LISTEN(EventType_MotionCompensation);
    #undef LISTEN
    return v;
}

static vector<enum EventType> g_eventsToListen = EVENTS_TO_LISTEN_INITIALIZER();


MfllFeatureDump::MfllFeatureDump(void)
    : m_filepath(MFLL_DUMP_DEFAUL_PATH)
{
    memset(&m_dumpFlag, 0, sizeof(m_dumpFlag));
}

MfllFeatureDump::~MfllFeatureDump(void)
{
}

void MfllFeatureDump::onEvent(
        enum EventType      /* t */,
        MfllEventStatus_t&  /* status */,
        void*               /* mfllCore */,
        void*               /* param1 */,
        void*               /* param2 */
        )
{
}

void MfllFeatureDump::doneEvent(enum EventType t, MfllEventStatus_t &status, void *mfllCore, void *param1, void* param2)
{
    enum MfllErr err = MfllErr_Ok;
    IMfllCore *c = static_cast<IMfllCore*>(mfllCore);
    unsigned int index = (unsigned int)(long)param1;
    MfllMiddlewareInfo_t info = c->getMiddlewareInfo();
    info.frameKey = static_cast<int>(index);

    mfllLogD("doneEvent: %s", IMfllCore::getEventTypeName(t));
    mfllLogD("status.ignore = %d", status.ignore);


    switch(t) {
    case EventType_Capture:
        break;

    case EventType_CaptureMss:
        dumpMssStage((IMfllCore*)(mfllCore), (int)(long)param1);
        break;

    case EventType_Blending:
        dumpMsfStage((IMfllCore*)(mfllCore), (int)(long)param1);
        break;
    case EventType_MotionCompensation:
        if (m_dumpFlag.yuv){
#if 0
            if (c->isFullSizeMc()) {
                __dump_image(
                        c->retrieveBuffer(MfllBuffer_FullSizeYuv, index + 1).get(),
                        MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                        "memc",
                        "yuv",
                        "nv12",
                        info);
            }
            else {
                __dump_image(
                        c->retrieveBuffer(MfllBuffer_QYuv, index + 1).get(),
                        MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                        "memc",
                        "yuv",
                        "nv12",
                        info);
            }
#endif
            // dump confidence map if exists
            auto confMap = c->retrieveBuffer(MfllBuffer_ConfidenceMap, index); // start from index 0
            if (confMap.get()) {
                __dump_image(
                        confMap.get(),
                        MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                        "memc",
                        "conf",
                        "y",
                        info,
                        false);
            }

            // dump motion compensation mv if exists
            auto mcMv = c->retrieveBuffer(MfllBuffer_MationCompensationMv, index); // start from index 0
            if (mcMv.get()) {
                __dump_image(
                        mcMv.get(),
                        MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                        "memc",
                        "mcmv",
                        "sta32",
                        info,
                        false);
            }
        }
    default:
        // do nothing
        break;
    } // switch(t)
}

vector<enum EventType> MfllFeatureDump::getListenedEventTypes(void)
{
    return g_eventsToListen;
}

void MfllFeatureDump::dump_image(
        IMfllImageBuffer*   /* pImg */,
        const DumpStage&    /* stage */,
        const char*         /* name */,
        int                 /* number */)
{
}

void MfllFeatureDump::dumpCapture(IMfllCore* /* pCore */)
{
}

void MfllFeatureDump::dumpBaseStage(IMfllCore* /* pCore */)
{
}

void MfllFeatureDump::dumpMfbStage(IMfllCore* /* pCore */, int /* index */)
{
}

void MfllFeatureDump::dumpMssStage(IMfllCore* pCore, int index)
{
    enum MfllErr err = MfllErr_Ok;
    MfllCore *c = (MfllCore*)pCore;
    MfllMiddlewareInfo_t info = pCore->getMiddlewareInfo();
    info.frameKey = index;

    if (m_dumpFlag.mfb == 0)
        return;

    mfllAutoLogFunc();

    auto dp = [&](IMfllImageBuffer* pBuf, const char* bufferName, bool commonDump, const char* extName = nullptr)
    {
        __dump_image(pBuf, MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT, "mss", bufferName, extName, info, commonDump);
    };

    /* mss frame */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE ; stage++) {
        string name = "ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_MssYuv, index, stage).get(), name.c_str(), true/*, "nv12"*/);
    }

    if (index > 0) {
        // dump motion compensation mv if exists
        auto mcMv = c->retrieveBuffer(MfllBuffer_MationCompensationMv, index-1); // start from index 0
        if (mcMv.get()) {
            __dump_image(
                    mcMv.get(),
                    MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                    "mss",
                    "mcmv",
                    "sta32",
                    info,
                    false);
        }

        /* difference frame*/
        dp( c->retrieveBuffer(MfllBuffer_FullSizeYuv, index).get(), "omc", true/*, "nv12"*/);
    }
}

void MfllFeatureDump::dumpMsfStage(IMfllCore* pCore, int index)
{
    enum MfllErr err = MfllErr_Ok;
    MfllCore *c = (MfllCore*)pCore;
    MfllMiddlewareInfo_t info = pCore->getMiddlewareInfo();
    info.frameKey = index;

    if (m_dumpFlag.mfb == 0) {
        /* last msf stage */
        if (index+2 >= pCore->getFrameCapturedNum())
            dumpMixStage(pCore);
        return;
    }

    mfllAutoLogFunc();

    auto dp = [&](IMfllImageBuffer* pBuf, const char* bufferName, bool commonDump, const char* extName = nullptr)
    {
        __dump_image(pBuf, MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT, "msf", bufferName, extName, info, commonDump);
    };

    /* base frame */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE ; stage++) {
        string name = "base-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_BaseYuv, index, stage).get(), name.c_str(), true/*, "nv12"*/);
    }

    /* reference frame */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE ; stage++) {
        string name = "ref-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_ReferenceYuv, index, stage).get(), name.c_str(), true/*, "nv12"*/);
    }

    /* weighting input */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE-1 ; stage++) {
        string name = "wt_in-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_WeightingIn, index, stage).get(), name.c_str(), false, "y");
    }

    /* weighting output */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE-1 ; stage++) {
        string name = "wt_out-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_WeightingOut, index, stage).get(), name.c_str(), false, "y");
    }

    /* weighting ds */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE-1 ; stage++) {
        string name = "wt_ds-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_WeightingDs, index, stage).get(), name.c_str(), false, "y");
    }

    /* output frame */
    for(int stage = 0 ; stage < MFLL_PYRAMID_SIZE ; stage++) {
        string name = "blended-ps" + to_string(stage);
        dp( c->retrievePyramidBuffer(MfllBuffer_BlendedYuv, index, stage).get(), name.c_str(), true/*, "nv12"*/);
    }

    /* difference frame*/
    dp( c->retrieveBuffer(MfllBuffer_DifferenceImage).get(), "idi", true/*, "nv12"*/);

    /* confidence map */
    auto confMap = c->retrieveBuffer(MfllBuffer_ConfidenceMap, index); // start from index 0
    if (confMap.get()) {
        __dump_image(
                confMap.get(),
                MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT,
                "msf",
                "conf",
                "y",
                info,
                false);
    }

    /* ds frame = base s6 */
    dp( c->retrievePyramidBuffer(MfllBuffer_BaseYuv, index, 6).get(), "ds", true/*, "nv12"*/);

    /* last msf stage */
    if (index+2 >= pCore->getFrameCapturedNum())
        dumpMixStage(pCore);
}

void MfllFeatureDump::dumpMixStage(IMfllCore *pCore)
{
    MfllCore *c = (MfllCore*)pCore;
    MfllMiddlewareInfo_t info = pCore->getMiddlewareInfo();

    if (m_dumpFlag.mix == 0)
        return;

    mfllAutoLogFunc();

    auto dp = [&](IMfllImageBuffer* pBuf, const char* bufferName, bool commonDump, const char* extName = nullptr)
    {
        __dump_image(pBuf, MFLL_DUMP_FILE_NAME_OTHER_ALIGNMENT, "afbld", bufferName, extName, info, commonDump);
    };

    /* output frame */
    if (c->getMixingBufferType() == MixYuvType_Working) {
        dp( c->retrieveBuffer(MfllBuffer_MixedYuv).get(), "mixed_working", true);
    }
    else {
        dp( c->retrieveBuffer(MfllBuffer_MixedYuv).get(), "mixed", true/*, "nv12"*/);
    }


    if (m_dumpFlag.mix == 2) {
        mfllLogD("Dump mix out only");
        return;
    }

    /* output debug frame*/
    dp( c->retrieveBuffer(MfllBuffer_MixedYuvDebug).get(), "mixed_debug", true);

    /* dceso */
    dp( c->retrieveBuffer(MfllBuffer_DCESOWorking).get(), "dceso", false, "sta32");

}
