#ifndef __IMSNRCORE_H__
#define __IMSNRCORE_H__

// AOSP
#include <utils/RefBase.h> // android::RefBase
// STD
#include <memory>
#include <vector>
#include <string>
#include <deque> // std::deque
//
// MTKCAM
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
// MTK utils
#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#include <mtkcam/utils/metadata/IMetadata.h>


using android::sp;
using std::vector;
using namespace NSCam;

enum MsnrErr {
    MsnrErr_Ok = 0,
    MsnrErr_Shooted,
    MsnrErr_AlreadyExist,
    MsnrErr_NotInited,
    MsnrErr_BadArgument,
};

enum MsnrMode {
    MsnrMode_Disable = 0,
    MsnrMode_Raw,
    MsnrMode_YUV,
};

class IMsnr {
public:
    struct ConfigParams{
        unsigned int openId;
        MSize        buffSize;
        MBOOL        isPhysical;
        MBOOL        isDualMode;
        MBOOL        isBayerBayer;
        MBOOL        isBayerMono;
        MBOOL        isMasterIndex;
        MBOOL        hasDCE;
        MBOOL        isVSDoFMode;
        MSize        fovBufferSize;
        MRect        fovCropRegion;
        IMetadata    *appMeta;
        IMetadata    *halMeta;
        IMetadata    *appDynamic; //P1 dynamic app
        IImageBuffer *inputBuff;
        IImageBuffer *inputRSZBuff;
        IImageBuffer *inputLCSBuff;
        IImageBuffer *inputLCESHOBuff;
        IImageBuffer *outputBuff;
        void         *bufLce2Caltm;
        ConfigParams()
            : openId(0)
            , buffSize(0)
            , isPhysical(MFALSE)
            , isDualMode(MFALSE)
            , isBayerBayer(MFALSE)
            , isBayerMono(MFALSE)
            , isMasterIndex(MTRUE)
            , hasDCE(MTRUE)
            , isVSDoFMode(MFALSE)
            , fovBufferSize(MSize(0, 0))
            , fovCropRegion(MRect(0, 0))
            , appMeta(nullptr)
            , halMeta(nullptr)
            , appDynamic(nullptr)
            , inputBuff(nullptr)
            , inputRSZBuff(nullptr)
            , inputLCSBuff(nullptr)
            , inputLCESHOBuff(nullptr)
            , outputBuff(nullptr)
            , bufLce2Caltm(nullptr)
            {};
    };
    // input parameters for getMsnrMode API
    struct ModeParams{
        MINT32 openId = 0;
        MBOOL  isRawInput     = MTRUE;
        MBOOL  isHidlIsp      = MFALSE;
        MBOOL  isPhysical     = MTRUE;
        MBOOL  isDualMode     = MFALSE;
        MBOOL  isBayerBayer   = MFALSE;
        MBOOL  isBayerMono    = MFALSE;
        MBOOL  isMaster       = MTRUE;
        MINT32 multiframeType = -1;
        MBOOL  needRecorder   = MFALSE;
        IMetadata* pHalMeta   = nullptr;
        IMetadata* pDynMeta   = nullptr;
        IMetadata* pAppMeta   = nullptr;
        IMetadata* pSesMeta   = nullptr;
    };
    // input parameters for decideMsnrMode API
    struct DecideModeParams{
        MINT32 openId         = 0;
        MBOOL  isRawInput     = MTRUE;
        MINT32 multiframeType = -1;
        MBOOL  needRecorder   = MFALSE;
        IMetadata* pHalMeta   = nullptr;
        IMetadata* pAppMeta   = nullptr;
    };
public:
    /**
     *  Caller should always create instance via INTERFACE::createInstance method.
     *  The lifetime of instance is controlled by the shared_ptr.
     *
     *  @return             - An IMsnr instance, caller has responsibility to manager it's lifetime.
     */
    static std::shared_ptr<IMsnr> createInstance(enum MsnrMode mode = MsnrMode_Raw);
    /**
     *  getMsnrMode() will call decideMsnrMode() to decide Raw or YUV mode or Off mode
     *
     *  @return             - Msnrmode that indicates current msnrmode is off /raw in /yuv in.
     */
    static enum MsnrMode getMsnrMode(const ModeParams &param);
    /**
     *  decideMsnrMode() only decide Raw or YUV mode, it's simple version of getMsnrMode()
     *  This API is expected to be used only inside msnr algorithm flow.
     *
     *  @return             - Msnrmode that indicates current msnrmode is raw in /yuv in.
     */
    static enum MsnrMode decideMsnrMode(const DecideModeParams &decideparam);

public:
    /**
     *  Init MsnrCore, if using MrpMode_BestPerformance, a thread will be create
     *  to allocate memory chunks, notice that, this thread will be joined while
     *  destroying MsnrCore.
     *  @param cfg          - MsnrConfig_t structure to describe usage
     *  @return             - Returns MsnrErr_Ok if works
     */
    virtual enum MsnrErr init (const ConfigParams &cfg) = 0;
    virtual enum MsnrErr doMsnr () = 0;
    virtual enum MsnrErr makeDebugInfo(IMetadata* metadata) = 0;



protected:
    virtual ~IMsnr(void) {};

}; /* class IMsnr */
#endif

