//#define LOG_NDEBUG 0
#define LOG_TAG "StereoInfoAccessor/PackerManager"

#include "PackerManager.h"
#include "StereoLog.h"
#include "XmpPacker.h"
#include "CustomizedDataPacker.h"
#include "JpgPacker.h"
#include <utils/Trace.h>

using namespace stereo;

#ifdef ATRACE_TAG
#undef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#else
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#endif

StereoBuffer_t PackerManager::pack(PackInfo *packInfo) {
    ATRACE_NAME(">>>>PackerManager-pack");
    StereoLogD("<PackerManager::pack begin>");
    // 1. append xmp format
    XmpPacker xmpPacker(packInfo);
    xmpPacker.pack();
    // 2. append cust format
    CustomizedDataPacker custDataPacker(packInfo);
    custDataPacker.pack();
    // 3. append jpg format
    JpgPacker jpgPacker(packInfo);
    jpgPacker.pack();
    StereoLogD("<PackerManager::pack done>");
    return packInfo->packedJpgBuf;
}

PackInfo* PackerManager::unpack(StereoBuffer_t &src) {
    ATRACE_NAME(">>>>PackerManager-unpack");
    StereoLogD("<unpack>");
    PackInfo *packInfo = new PackInfo();
    packInfo->packedJpgBuf = src;
    // 1. split jpg format
    JpgPacker jpgPacker(packInfo);
    jpgPacker.unpack();
    // 2. split xmp format
    XmpPacker xmpPacker(packInfo);
    xmpPacker.unpack();
    // 3. split cust format
    CustomizedDataPacker custDataPacker(packInfo);
    custDataPacker.unpack();

    return packInfo;
}

