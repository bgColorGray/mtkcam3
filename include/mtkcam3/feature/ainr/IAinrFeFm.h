#ifndef __IAINRFEFM_H__
#define __IAINRFEFM_H__

#include "AinrDefs.h"
#include "AinrTypes.h"

// STL
#include <memory>
//
/**
 *  IFeFm has responsibility handle two input buffers (Base frame and referenced frame)
 *  matched data. Ususally used by algorithm to do homography postprocessing. IFeFm output
 *  data FMO would indicate matching point of two frames. Be aware of that FMO is confidential
 *  format we should parsing result by algorithm before using it.
 */
using namespace NSCam;

namespace ainr {
class IAinrFeFm {
public:
    struct FeoInfo {
        MSize rrzoSize;
        int feoTotalSize;
        FeoInfo()
            : feoTotalSize(0)
            {};
    };

    struct ConfigParams
    {
        int     openId;
        MSize   bufferSize;
        int     captureNum;
        int     index;
        std::function<void(int)> cbMethod;
        ConfigParams()
            : openId(0)
            , bufferSize(MSize())
            , captureNum(0)
            , index(0)
        {};
        ConfigParams(int sensorId)
            : openId(sensorId)
            , bufferSize(MSize())
            , captureNum(0)
            , index(0)
        {};
    };

    struct OutInfo
    {
        MRect resizeCrop;
        MSize feoSize;
        MSize fmoSize;
    };

    //
    struct BufferPackage
    {
        IImageBuffer *inBase;
        IImageBuffer *inRef;
        IImageBuffer *outFeoBase;
        IImageBuffer *outFeoRef;
        IImageBuffer *outFmoBase;
        IImageBuffer *outFmoRef;
        BufferPackage()
            : inBase(nullptr)
            , inRef(nullptr)
            , outFeoBase(nullptr)
            , outFeoRef(nullptr)
            , outFmoBase(nullptr)
            , outFmoRef(nullptr)
            {};
    };
    struct MetaDataPackage
    {
        // Metadata base frame
        IMetadata* appMetaBase;
        IMetadata* halMetaBase;
        IMetadata* dynamicMetaBase; //p1node output app meta
        // Metadata ref frame
        IMetadata* appMetaRef;
        IMetadata* halMetaRef;
        IMetadata* dynamicMetaRef; //p1node output app meta
        MetaDataPackage()
            : appMetaBase(nullptr)
            , halMetaBase(nullptr)
            , dynamicMetaBase(nullptr)
            , appMetaRef(nullptr)
            , halMetaRef(nullptr)
            , dynamicMetaRef(nullptr)
            {};
    };
    struct DataPackage
    {
        // Metadata base frame
        IMetadata    *appMeta;
        IMetadata    *halMeta;
        IImageBuffer *inBuf;
        IImageBuffer *outBuf;
        // It is used for bittrue and it is obtional
        IImageBuffer *outYuv;
        std::vector<int> *matrix;
        DataPackage()
            : appMeta(nullptr)
            , halMeta(nullptr)
            , inBuf(nullptr)
            , outBuf(nullptr)
            , outYuv(nullptr)
            , matrix(nullptr)
            {};
    };
public:
    static std::shared_ptr<IAinrFeFm> createInstance(void);

/* interfaces */
public:

    /* Feed config data into FEFM.
     *  @param [in] params capture number/openId....
     *  @param [out] outParam crop/feo/fmo information
     */
    virtual enum AinrErr init(IAinrFeFm::ConfigParams const& params,
            IAinrFeFm::OutInfo *outParam = nullptr) = 0;

    /* Get FEFM output buffer size. Should be called after init.
     *  @param feoSize [out] feo size
     *  @param fmoSize [out] fmo size
     *
     */
    virtual enum AinrErr getFeFmSize(MSize &feoSize, MSize &fmoSize) = 0;

    /* Build up FE request before doFEFM
     *  @param [in] package pass in/out and meta
     *  @return return status
     */
    virtual enum AinrErr buildFe(IAinrFeFm::DataPackage *package) = 0;

    /* Build up FM request before doFEFM
     *  @param [in] baseBuf Base buffer you want to match.
     *  @param [in] refBuf Reference buffer.
     *  @param [out] fmo Output fmo(feature matching) buffer.
     *  @param [out] index request index.
     *  @param needCb need early callback or not.
     *  @return                     Address of the buffer, if read fail returns
     */
    virtual enum AinrErr buildFm(IImageBuffer *baseBuf, IImageBuffer *refBuf, IImageBuffer *fmo, int index, bool needCb) = 0;

    /* Nonblocking API. Enque FE input buffer to Pass2. Need to waitDoFeDone before using FE buffers
     *  @param [out] package pass in/out and meta
     *
     *  @return                     return status
     *
     */
    virtual enum AinrErr doFe(const IAinrFeFm::DataPackage *package) = 0;

    /* Wait fe deque done. Must be called after doFe.
     *  @return                     return status
     *
     */
    virtual enum AinrErr waitFeDone() = 0;

    /* Burst mode execute FEFM which has better performance.
     *  @return                     return status.
     */
    virtual enum AinrErr doFeFm() = 0;
protected:
    virtual ~IAinrFeFm() { };
};
}; /* namespace ainr */

#endif//__IAINRFEFM_H__