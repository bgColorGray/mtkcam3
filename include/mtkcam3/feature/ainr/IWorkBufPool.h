#ifndef __IWORKBUFPOOL_H__
#define __IWORKBUFPOOL_H__

#include "AinrTypes.h"

// AOSP
#include <utils/StrongPointer.h>
#include <mtkcam/utils/imgbuf/IIonDevice.h>

// STL
#include <memory>
#include <utility>  // std::pair

namespace ainr {

using SpImgBuffer = android::sp<NSCam::IImageBuffer>;

class IWorkBufPool {
public:
    struct BufferInfo {
        uint32_t width;
        uint32_t height;
        uint32_t format;
        uint32_t aligned;
        bool     isPixelAligned;
        explicit BufferInfo(uint32_t w, uint32_t h, uint32_t fmt)
            : width(w)
            , height(h)
            , format(fmt)
            , aligned(1)
            , isPixelAligned(false)
            {}
    };
    static std::shared_ptr<IWorkBufPool> createInstance(
            std::shared_ptr<NSCam::IIonDevice> spIonDevice = nullptr,
            MBOOL useCapFeaturePool = MFALSE);
    virtual ~IWorkBufPool(void) {};
/* interfaces */
public:
    /* Allocate working buffers
     *  @param isAsync                  Allocate buffers asynchronously or not.
     *  @param bufferCount              Allocate buffers' count.
     *  @return                         Return AinrErr_Ok if all buffers being allocated.
     */
    virtual enum AinrErr allocBuffers(const BufferInfo &info,
            bool isAsync, int32_t bufferCount) = 0;

    /* Acquire image buffer from pool.
     * Thread safe method.
     *  @param params       AiParam indicate basic buffer information.
     *  @return bufPackage   Input/Output buffers.
     */
    virtual android::sp<NSCam::IImageBuffer> acquireBufferFromPool() = 0;

    /* Release image buffer into pool.
     *  @param buf          Image buffer which you want to release it.
     *  @return             Return AinrErr_Ok if the buffer be released.
     */
    virtual void releaseBufferToPool(android::sp<NSCam::IImageBuffer> &buf) = 0;

    /* Release all acquired buffers back to pool.
     */
    virtual void releaseAllBuffersToPool() = 0;
};
}; /* namespace ainr */

#endif  //__IWORKBUFPOOL_H__
