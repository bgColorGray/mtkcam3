//#define LOG_NDEBUG 0
#define LOG_TAG "StereoInfoAccessor/StereoInfoAccessor"

#include <mtkcam3/feature/stereo/libstereoinfoaccessor/StereoInfoAccessor.h>
#include "StereoLog.h"
#include "Utils.h"
#include <utils/Trace.h>
#include "ParserFactory.h"
#include "PackerManager.h"
#include "StereoInfoJsonParser.h"
#include "ReadWriteLockFileUtils.h"

using namespace stereo;

#ifdef ATRACE_TAG
#undef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#else
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#endif

void StereoInfoAccessor::parseInfoAndWriteJPGHeader(
    IStereoInfo* stereoInfo,
    StereoBuffer_t &jpgBuffer,
    int infoType)
{
    ATRACE_NAME(">>>>StereoInfoAccessor-parseInfoAndWriteJPGHeader");
    long startTime = Utils::getCurrentTime();

    // dump original jpg buffer
    StereoString dumpPath;
    if (Utils::ENABLE_BUFFER_DUMP) {
        dumpPath.append(DUMP_FILE_FOLDER).append("/").append(stereoInfo->debugDir).append("/");
        Utils::writeBufferToFile(dumpPath.append("StereoInfo_oriJpgBuffer_write.jpg"),
            stereoInfo->jpgBuffer);
    }

    PackInfo packInfo;
    PackerManager packerManager;
    packInfo.unpackedJpgBuf = stereoInfo->jpgBuffer;

    // create parser
    IParser *parser = ParserFactory::getParserInstance(
        infoType, stereoInfo, packInfo.unpackedStandardXmpBuf,
        packInfo.unpackedExtendedXmpBuf, packInfo.unpackedCustomizedBufMap, packInfo.md5);
    if (parser == nullptr) {
        StereoLogE("<parseInfoAndWriteJPGHeader> can not create parser");
        return;
    }

    // parser write
    parser->write();
    // serialize
    serialize(packInfo, *parser);
    // pack
    jpgBuffer = packerManager.pack(&packInfo);
    // dump packed jpg buffer
    if (Utils::ENABLE_BUFFER_DUMP) {
        dumpPath.clear();
        dumpPath.append(DUMP_FILE_FOLDER).append("/").append(stereoInfo->debugDir).append("/");
        Utils::writeBufferToFile(
            dumpPath.append("StereoInfo_packedJpgBuffer_write.jpg"), jpgBuffer);
    }

    // release
    delete parser;
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<parseInfoAndWriteJPGHeader> end, elapsed time = %ld ms", elapsedTime);
}

void StereoInfoAccessor::serialize(PackInfo &info, IParser &parser) {
    ATRACE_NAME(">>>>StereoInfoAccessor-serialize");
    SerializedInfo *serializedInfo = parser.serialize();
    if(serializedInfo == nullptr) return;
    info.unpackedStandardXmpBuf = serializedInfo->standardXmpBuf;
    info.unpackedExtendedXmpBuf = serializedInfo->extendedXmpBuf;
    info.unpackedCustomizedBufMap = serializedInfo->customizedBufMap;
    delete serializedInfo;
}

void StereoInfoAccessor::writeStereoDepthInfo(
        const StereoString &filePath, StereoDepthInfo &depthInfo) {
    ATRACE_NAME(">>>>StereoInfoAccessor-writeStereoDepthInfo");
    StereoLogI("<writeStereoDepthInfo> filePath: %s, captureInfo: %s",
        filePath.c_str(), depthInfo.toString().c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::writeLock(filePath);

    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<writeStereoDepthInfo> file buffer is null");
        return;
    }

    // set debug dir
    depthInfo.debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;

    // create parser
    IParser *stereoDepthInfoParser = ParserFactory::getParserInstance(
        STEREO_DEPTH_INFO, &depthInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (stereoDepthInfoParser == nullptr) {
        StereoLogE("<writeStereoDepthInfo> can not create parser");
        return;
    }
    // parser write
    stereoDepthInfoParser->write();
    // serialize
    serialize(*pPackInfo, *stereoDepthInfoParser);
    // pack
    packerManager.pack(pPackInfo);

    Utils::writeBufferToFile(filePath, pPackInfo->packedJpgBuf);

    // release
    delete stereoDepthInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<writeStereoDepthInfo> end, elapsed time = %ld ms", elapsedTime);
}

void StereoInfoAccessor::writeSegmentMaskInfo(
        const StereoString &filePath, SegmentMaskInfo &maskInfo) {
    ATRACE_NAME(">>>>StereoInfoAccessor-writeSegmentMaskInfo");
    StereoLogI("<writeSegmentMaskInfo> filePath: %s, depthInfo: %s",
        filePath.c_str(), maskInfo.toString().c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::writeLock(filePath);

    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<writeSegmentMaskInfo> file buffer is null");
        return;
    }

    // set debug dir
    maskInfo.debugDir = Utils::getFileNameFromPath(filePath);
    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;
    // create parser
    IParser *segmentMaskInfoParser = ParserFactory::getParserInstance(
        SEGMENT_MASK_INFO, &maskInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (segmentMaskInfoParser == nullptr) {
        StereoLogE("<writeSegmentMaskInfo> can not create parser");
        return;
    }
    // parser write
    segmentMaskInfoParser->write();
    // serialize
    serialize(*pPackInfo, *segmentMaskInfoParser);
    // pack
    packerManager.pack(pPackInfo);

    Utils::writeBufferToFile(filePath, pPackInfo->packedJpgBuf);

    // release
    delete segmentMaskInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<writeSegmentMaskInfo> end, elapsed time = %ld ms", elapsedTime);
}

void StereoInfoAccessor::writeRefocusImage(
        const StereoString &filePath, StereoConfigInfo &configInfo, StereoBuffer_t &blurImage) {
    ATRACE_NAME(">>>>StereoInfoAccessor-writeRefocusImage");
    StereoLogI("<writeRefocusImage> filePath: %s, StereoConfigInfo: %s",
        filePath.c_str(), configInfo.toString().c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::writeLock(filePath);

    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<writeRefocusImage> file buffer is null");
        return;
    }

    // set debug dir
    configInfo.debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = NULL;
    if(fileBuffer.size > 0){
        pPackInfo = packerManager.unpack(fileBuffer);
        pPackInfo->unpackedJpgBuf = fileBuffer;
    }
    else{
        return;
    }

    // create parser
    IParser *stereoConfigInfoParser = ParserFactory::getParserInstance(
        STEREO_CONFIG_INFO, &configInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (stereoConfigInfoParser == nullptr) {
        StereoLogE("<writeRefocusImage> can not create parser");
        return;
    }
    // parser write
    stereoConfigInfoParser->write();
    // serialize
    serialize(*pPackInfo, *stereoConfigInfoParser);
    pPackInfo->unpackedBlurImageBuf = blurImage;
    // pack
    packerManager.pack(pPackInfo);

    Utils::writeBufferToFile(filePath, pPackInfo->packedJpgBuf);

    // release
    delete stereoConfigInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<writeRefocusImage> end, elapsed time = %ld ms", elapsedTime);
}

StereoDepthInfo* StereoInfoAccessor::readStereoDepthInfo(const StereoString & filePath) {
    ATRACE_NAME(">>>>StereoInfoAccessor-readStereoDepthInfo");
    StereoLogI("<readStereoDepthInfo> filePath: %s", filePath.c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::readLock(filePath);

    StereoDepthInfo *pInfo = new StereoDepthInfo();
    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<readStereoDepthInfo> file buffer is null");
        delete pInfo;
        return nullptr;
    }

    // set debug dir
    pInfo->debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;

    // create parser
    IParser *stereoDepthInfoParser = ParserFactory::getParserInstance(
        STEREO_DEPTH_INFO, pInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (stereoDepthInfoParser == nullptr) {
        StereoLogE("<readStereoDepthInfo> can not create parser");
        return nullptr;
    }
    // parser write
    stereoDepthInfoParser->read();

    // release
    delete stereoDepthInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<readStereoDepthInfo> end, elapsed time = %ld ms", elapsedTime);
    return pInfo;
}

SegmentMaskInfo* StereoInfoAccessor::readSegmentMaskInfo(const StereoString & filePath) {
    ATRACE_NAME(">>>>StereoInfoAccessor-readSegmentMaskInfo");
    StereoLogI("<readSegmentMaskInfo> filePath: %s", filePath.c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::readLock(filePath);

    SegmentMaskInfo *pInfo = new SegmentMaskInfo();
    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<readSegmentMaskInfo> file buffer is null");
        delete pInfo;
        return nullptr;
    }

    // set debug dir
    pInfo->debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;

    // create parser
    IParser *segmentMaskInfoParser = ParserFactory::getParserInstance(
        SEGMENT_MASK_INFO, pInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (segmentMaskInfoParser == nullptr) {
        StereoLogE("<readSegmentMaskInfo> can not create parser");
        return nullptr;
    }
    // parser write
    segmentMaskInfoParser->read();

    // release
    delete segmentMaskInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<readSegmentMaskInfo> end, elapsed time = %ld ms", elapsedTime);
    return pInfo;
}

StereoBufferInfo* StereoInfoAccessor::readStereoBufferInfo(const StereoString & filePath) {
    ATRACE_NAME(">>>>StereoInfoAccessor-readStereoBufferInfo");
    StereoLogI("<readStereoBufferInfo> filePath: %s", filePath.c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::readLock(filePath);

    StereoBufferInfo *pInfo = new StereoBufferInfo();
    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<readStereoBufferInfo> file buffer is null");
        delete pInfo;
        return nullptr;
    }

    // set debug dir
    pInfo->debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;

    // create parser
    IParser *stereoBufferInfoParser = ParserFactory::getParserInstance(
        STEREO_BUFFER_INFO, pInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (stereoBufferInfoParser == nullptr) {
        StereoLogE("<readStereoBufferInfo> can not create parser");
        return nullptr;
    }
    // parser write
    stereoBufferInfoParser->read();

    // release
    delete stereoBufferInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<readStereoBufferInfo> end, elapsed time = %ld ms", elapsedTime);
    return pInfo;
}

StereoConfigInfo* StereoInfoAccessor::readStereoConfigInfo(const StereoString & filePath) {
    ATRACE_NAME(">>>>StereoInfoAccessor-readStereoConfigInfo");
    StereoLogI("<readStereoConfigInfo> filePath: %s", filePath.c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::readLock(filePath);

    StereoConfigInfo *pInfo = new StereoConfigInfo();
    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<readStereoConfigInfo> file buffer is null");
        delete pInfo;
        return nullptr;
    }

    // set debug dir
    pInfo->debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = NULL;
    if(fileBuffer.size > 0){
        pPackInfo = packerManager.unpack(fileBuffer);
        pPackInfo->unpackedJpgBuf = fileBuffer;
    }
    else{
        return nullptr;
    }
    // create parser
    IParser *stereoConfigInfoParser = ParserFactory::getParserInstance(
        STEREO_CONFIG_INFO, pInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (stereoConfigInfoParser == nullptr) {
        StereoLogE("<readStereoConfigInfo> can not create parser");
        return nullptr;
    }
    // parser write
    stereoConfigInfoParser->read();

    // release
    delete stereoConfigInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<readStereoConfigInfo> end, elapsed time = %ld ms", elapsedTime);
    return pInfo;
}

GoogleStereoInfo* StereoInfoAccessor::readGoogleStereoInfo(const StereoString & filePath) {
    ATRACE_NAME(">>>>StereoInfoAccessor-readGoogleStereoInfo");
    StereoLogI("<readGoogleStereoInfo> filePath: %s", filePath.c_str());
    long startTime = Utils::getCurrentTime();

    ReadWriteLockFileUtils::readLock(filePath);

    GoogleStereoInfo *pInfo = new GoogleStereoInfo();
    // read file to buffer
    PackerManager packerManager;
    StereoBuffer_t fileBuffer;
    Utils::readFileToBuffer(filePath, fileBuffer);
    if (!fileBuffer.isValid()) {
        StereoLogE("<readGoogleStereoInfo> file buffer is null");
        delete pInfo;
        return nullptr;
    }

    // set debug dir
    pInfo->debugDir = Utils::getFileNameFromPath(filePath);

    // unpack file buffer
    PackInfo *pPackInfo = packerManager.unpack(fileBuffer);
    pPackInfo->unpackedJpgBuf = fileBuffer;

    // create parser
    IParser *googleStereoInfoParser = ParserFactory::getParserInstance(
        GOOGLE_STEREO_INFO, pInfo, pPackInfo->unpackedStandardXmpBuf,
        pPackInfo->unpackedExtendedXmpBuf, pPackInfo->unpackedCustomizedBufMap, pPackInfo->md5);
    if (googleStereoInfoParser == nullptr) {
        StereoLogE("<readGoogleStereoInfo> can not create parser");
        return nullptr;
    }
    // parser write
    googleStereoInfoParser->read();

    // release
    delete googleStereoInfoParser;
    delete pPackInfo;

    ReadWriteLockFileUtils::unlock(filePath);
    long elapsedTime = Utils::getCurrentTime() - startTime;
    StereoLogI("<readGoogleStereoInfo> end, elapsed time = %ld ms", elapsedTime);
    return pInfo;
}

int StereoInfoAccessor::getGeoVerifyLevel(const StereoBuffer_t &configBuffer) {
    if (!configBuffer.isValid()) {
        StereoLogW("<getGeoVerifyLevel> configBuffer is null!!");
        return -1;
    }
    ATRACE_NAME(">>>>StereoInfoAccessor-getGeoVerifyLevel");
    StereoInfoJsonParser parser(configBuffer);
    return parser.getGeoVerifyLevel();
}

int StereoInfoAccessor::getPhoVerifyLevel(const StereoBuffer_t &configBuffer) {
    if (!configBuffer.isValid()) {
        StereoLogW("<getPhoVerifyLevel> configBuffer is null!!");
        return -1;
    }
    ATRACE_NAME(">>>>StereoInfoAccessor-getPhoVerifyLevel");
    StereoInfoJsonParser parser(configBuffer);
    return parser.getPhoVerifyLevel();
}

int StereoInfoAccessor::getMtkChaVerifyLevel(const StereoBuffer_t &configBuffer) {
    if (!configBuffer.isValid()) {
        StereoLogW("<getMtkChaVerifyLevel> configBuffer is null!!");
        return -1;
    }
    ATRACE_NAME(">>>>StereoInfoAccessor-getMtkChaVerifyLevel");
    StereoInfoJsonParser parser(configBuffer);
    return parser.getMtkChaVerifyLevel();
}

StereoInfoAccessor::~StereoInfoAccessor() {
    BufferManager::releaseAll();
}
