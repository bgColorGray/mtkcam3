/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "ZSDHDRSelector"

#include "ZSDHDRSelector.h"

#include <stdlib.h>
#include <cutils/properties.h>

#include <vector>

#include <mtkcam3/pipeline/hwnode/StreamId.h>

#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if (            (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if (            (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if (            (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( CC_UNLIKELY(cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace Custom {
/******************************************************************************
 *
 ******************************************************************************/
ZSDHDRSelector::
ZSDHDRSelector() {
    mMode = ::property_get_int32("vendor.debug.camera.zsdhdr.mode", 0);

    initRequestTable();
}

/******************************************************************************
 *
 ******************************************************************************/
const char*
ZSDHDRSelector::
getName() const
{
    return "ZSDHDRSelector";
}

/******************************************************************************
 *
 ******************************************************************************/
ParsedZslRequestInfo
ZSDHDRSelector::
parseZslRequestInfo(ZslRequestInfo const& zslRequestInfo) const
{
    std::vector<ParsedZslRequestInfo> vParsedZslRequestInfo;

    auto const& frames = zslRequestInfo.frames;
    for (auto const& frame : frames) {
        ParsedZslRequestInfo tmp;
        tmp.requestNo = zslRequestInfo.requestNo;
        tmp.zslFrameIdx = frame.first;

        auto const& zslFrameInfo = frame.second;
        tmp.pRequiredStreams = zslFrameInfo.pRequired;

        auto const& ctrlMetas = zslFrameInfo.ctrlMetas;
        for (auto const& ctrlMeta : ctrlMetas) {
            auto streamId = ctrlMeta.first->getStreamId();
            if (streamId == eSTREAMID_META_APP_CONTROL) {
                auto const& meta = ctrlMeta.second;
                IMetadata::getEntry<MINT32>(
                        meta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_INDEX,
                        tmp.frameIndex);
                IMetadata::getEntry<MINT32>(
                        meta, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_FRAME_COUNT,
                        tmp.frameCount);
                if (tmp.frameCount > 0) {
                    if (tmp.frameIndex == 0) {
                        tmp.bIsFirstFrame = true;
                    }
                    if (tmp.frameIndex == tmp.frameCount - 1) {
                        tmp.bIsLastFrame = true;
                    }
                }
                break;
            }

            if (streamId == eSTREAMID_META_PIPE_CONTROL) {
                auto const& meta = ctrlMeta.second;
                IMetadata::getEntry<MINT32>(
                        meta, MTK_FEATURE_VHDR_STATUS, tmp.status);
            }
        }

        vParsedZslRequestInfo.push_back(tmp);

        MY_LOGD("[ParsedZslRequestInfo] requestNo: %d, "
                "frameIndex: %d, frameCount: %d, "
                "bIsFirstFrame: %d, bIsLastFrame: %d, "
                "status: %d",
                tmp.requestNo, tmp.frameIndex, tmp.frameCount,
                tmp.bIsFirstFrame, tmp.bIsLastFrame, tmp.status);
    }

    if (vParsedZslRequestInfo.size() > 1) {
        MY_LOGW("More than one frame is required from one ISPHIDL request!!!");
    }

    return vParsedZslRequestInfo.at(0);
}

/******************************************************************************
 *
 ******************************************************************************/
void ZSDHDRSelector::
dumpReqSetting() const
{
    for (auto const& reqSetting : mvReqSetting) {
        MY_LOGD("[ZSDHDRReqSetting] "
                "mOrder(%d) mExp(%d) mCategory(%d) "
                "mHisInfIdx(%d) mbClone(%d)",
                reqSetting.mOrder, reqSetting.mExp,
                reqSetting.mCategory, reqSetting.mHisInfIdx,
                reqSetting.mbClone);
    }

    MY_LOGD("mZSLHistoryCnt(%d) mZSLInflightCnt(%d)",
            mZSLHistoryCnt, mZSLInflightCnt);
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
decideReqSetting(ParsedZslRequestInfo const& parsedZslRequestInfo) const
{
    mvReqSetting =
            mRequestTable[std::make_tuple(mMode, parsedZslRequestInfo.status)];

    // extract mHisInfIdx, mZSLHistoryCnt, mZSLInflightCnt
    {
        auto compute = [&](std::vector<MINT8>& v,
                           std::map<MINT8, MINT8>& m,
                           MINT8& cnt) {
            std::sort(v.begin(), v.end());
            auto last = std::unique(v.begin(), v.end());
            v.erase(last, v.end());
            cnt = v.size();

            for (auto i = 0; i < v.size(); ++i) {
                m[v[i]] = i;
            }
        };

        std::map<MINT8, MINT8> m_history, m_inflight;
        std::vector<MINT8> v_history, v_inflight;
        for (auto const& reqSetting : mvReqSetting) {
            switch(reqSetting.mCategory) {
                case Category::eZSL_History:
                    v_history.push_back(reqSetting.mOrder);
                    break;
                case Category::eZSL_Inflight:
                    v_inflight.push_back(reqSetting.mOrder);
                    break;
                default:
                    break;
            }
        }
        compute(v_history, m_history, mZSLHistoryCnt);
        compute(v_inflight, m_inflight, mZSLInflightCnt);
        for (auto& reqSetting : mvReqSetting) {
            switch(reqSetting.mCategory) {
                case Category::eZSL_History:
                    reqSetting.mHisInfIdx = m_history[reqSetting.mOrder];
                    break;
                case Category::eZSL_Inflight:
                    reqSetting.mHisInfIdx = m_inflight[reqSetting.mOrder];
                    break;
                default:
                    break;
            }
        }
    }

    // extract mbClone
    {
        auto disableClone = [&](std::map<MINT8, MINT8>& m) {
            for (auto const& [key, value] : m) {
                mvReqSetting.at(value).mbClone = false;
            }
        };

        std::map<MINT8, MINT8> m_history, m_inflight;
        for (auto i = 0; i < mvReqSetting.size(); ++i) {
            switch(mvReqSetting.at(i).mCategory) {
                case Category::eZSL_History:
                    m_history[mvReqSetting.at(i).mHisInfIdx] = i;
                    break;
                case Category::eZSL_Inflight:
                    m_inflight[mvReqSetting.at(i).mHisInfIdx] = i;
                    break;
                default:
                    break;
            }
        }
        disableClone(m_history);
        disableClone(m_inflight);
    }

    mCounter = mvReqSetting.size();

    dumpReqSetting();
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
dump(SelectOutputParams const& out) const
{
    auto& selected = out.selected;
    for (auto it : selected) {
        auto& zslFrameIdx = it.first;
        auto& zslFrameResult = it.second;
        MY_LOGD("[SelectOutputParams] zslFrameIdx(%d) historyFrameNo(%d) clone(%d)",
                zslFrameIdx, zslFrameResult.frameNo, zslFrameResult.clone);
    }

    auto& operations = out.operations;
    for (auto it : operations) {
        MY_LOGD("[SelectOutputParams] historyFrameNo(%d) opt(%d)",
                it.first, it.second);
    }

    auto dumpRecordFrame = [&](std::vector<FrameInfo> const& vRecordFrame) {
        for (auto& recordFrame : vRecordFrame) {
            MY_LOGD("[RecordFrame] frameNo(%d) "
                    "NEIdx(%d) MEIdx(%d) SEIdx(%d) "
                    "isChecked(%d) isLocked(%d) "
                    "isZSD(%d)",
                    recordFrame.frameNo,
                    recordFrame.NEIdx, recordFrame.MEIdx, recordFrame.SEIdx,
                    recordFrame.isChecked, recordFrame.isLocked,
                    recordFrame.isZSD);
        }
    };
    dumpRecordFrame(mvRecordHistoryFrame);
    dumpRecordFrame(mvRecordInflightFrame);
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
extractBlobOrder(FrameInfo& frameInfo,
                 IMetadata const& meta) const
{
    IMetadata::IEntry entry = meta.entryFor(MTK_STAGGER_BLOB_IMGO_ORDER);
    if (!entry.isEmpty()) {
        MUINT8 entrySize = entry.count();
        for (int i = 0; i < entrySize; ++i) {
            auto tmp = entry.itemAt(i, Type2Type<MUINT8>());
            switch (tmp) {
                case MTK_STAGGER_IMGO_NE:
                    frameInfo.NEIdx = i;
                    break;
                case MTK_STAGGER_IMGO_ME:
                    frameInfo.MEIdx = i;
                    break;
                case MTK_STAGGER_IMGO_SE:
                    frameInfo.SEIdx = i;
                    break;
                default:
                    break;
            }
        }
    } else {
        MY_LOGW("Can not get MTK_STAGGER_BLOB_IMGO_ORDER!!!");
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
setBufferIndex(IMetadata& meta,
               FrameInfo const& frameInfo,
               MINT8 const& exp) const
{
    MINT32 bufferIndex = 0;
    switch (exp) {
        case MTK_STAGGER_IMGO_NE:
            bufferIndex = frameInfo.NEIdx;
            break;
        case MTK_STAGGER_IMGO_ME:
            bufferIndex = frameInfo.MEIdx;
            break;
        case MTK_STAGGER_IMGO_SE:
            bufferIndex = frameInfo.SEIdx;
            break;
        default:
            bufferIndex = frameInfo.NEIdx;
            break;
    }
    IMetadata::setEntry<MINT32>(
            &meta, MTK_FEATURE_VALID_IMAGE_INDEX, bufferIndex);
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
generateOut(SelectOutputParams& out,
            size_t const& zslFrameIdx,
            ZSDHDRReqSetting const& reqSetting,
            bool const& forceNonZSL) const
{
    ZslFrameResult zslFrameResult;

    IMetadata* pMeta = nullptr;
    auto additionalMeta = std::make_shared<IMetadata>();
    if (additionalMeta != nullptr) {
        pMeta = additionalMeta.get();
    }

    FrameInfo* pRecordFrame;
    bool bIsZSL = !forceNonZSL;
    switch (reqSetting.mCategory) {
        case Category::eZSL_History:
            pRecordFrame = &mvRecordHistoryFrame.at(reqSetting.mHisInfIdx);
            break;
        case Category::eZSL_Inflight:
            pRecordFrame = &mvRecordInflightFrame.at(reqSetting.mHisInfIdx);
            break;
        case Category::eNonZSL:
        default:
            bIsZSL = false;
            break;
    }

    if (bIsZSL) {
            zslFrameResult.frameNo = pRecordFrame->frameNo;

            zslFrameResult.clone = reqSetting.mbClone;

            setBufferIndex(*pMeta, *pRecordFrame, reqSetting.mExp);

            zslFrameResult.additionalMetas.emplace(
                    eSTREAMID_META_PIPE_DYNAMIC_01, additionalMeta);

            mCounter--;
    } else {
        zslFrameResult.frameNo = -1;

        IMetadata::setEntry<MINT32>(pMeta,
                MTK_3A_FEATURE_AE_TARGET_MODE,
                MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL);
        IMetadata::setEntry<MINT32>(pMeta,
                MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                MTK_STAGGER_VALID_EXPOSURE_1);
        IMetadata::setEntry<MINT32>(pMeta,
                MTK_3A_ISP_FUS_NUM,
                MTK_3A_ISP_FUS_NUM_1);
        zslFrameResult.additionalMetas.emplace(
                eSTREAMID_META_PIPE_CONTROL, additionalMeta);

        mCounter--;
    }

    out.selected.emplace(zslFrameIdx, zslFrameResult);
}

/******************************************************************************
 *
 ******************************************************************************/
bool
ZSDHDRSelector::
selectFromHistory(std::vector<FrameInfo>& vSelected,
                  int32_t const& requiredCnt,
                  std::map<HistoryFrameNoT, HistoryFrame> const& historyFrames,
                  int64_t const& zsdTimestamp) const
{
    bool ret = true;

    if (requiredCnt <= 0) {
        MY_LOGD("No need to select from history frames");
        return ret;
    }
    MY_LOGD("requiredCnt(%d)", requiredCnt);

    uint32_t zsdFrameNo = 0;
    int64_t pre_diff = -1;
    for (auto const& it : historyFrames) {
        auto const& historyFrameNo = it.first;
        auto const& historyFrame = it.second;
        auto diff = std::abs(historyFrame.sensorTimestampNs - zsdTimestamp);
        if (pre_diff < 0 || diff < pre_diff) {
            pre_diff = diff;
            zsdFrameNo = historyFrameNo;
        }
    }

    std::vector<FrameInfo> candidates;
    for (auto& it : historyFrames) {
        auto& historyFrameNo = it.first;
        auto& historyFrame = it.second;

        auto& halMetas = historyFrame.halMetas;
        for (auto& halMeta : halMetas) {
            auto streamId = halMeta.first->getStreamId();
            if (streamId == eSTREAMID_META_PIPE_DYNAMIC_01) {
                auto& pMeta = halMeta.second;

                MINT32 validExpNum = -1;
                if (!IMetadata::getEntry<MINT32>(
                        pMeta, MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM, validExpNum)) {
                    MY_LOGW("Can not get MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM!!!");
                }

                if (validExpNum == MTK_STAGGER_VALID_EXPOSURE_3) {
                    FrameInfo candidate;
                    candidate.frameNo = historyFrameNo;

                    if (historyFrameNo == zsdFrameNo) {
                        candidate.isZSD = true;
                    }

                    candidate.isChecked = true;

                    extractBlobOrder(candidate, *pMeta);

                    candidates.emplace_back(candidate);

                    MY_LOGD("[Candidate] historyFrameNo(%d) "
                            "frameNo(%d) requestNo(%d) "
                            "sensorTimestampNs(%ld) isZSD(%d)",
                            historyFrameNo,
                            historyFrame.frameNo, historyFrame.requestNo,
                            historyFrame.sensorTimestampNs, candidate.isZSD);
                }
                break;
            }
        }
    }

    if (candidates.size() >= requiredCnt) {
        for (int i = candidates.size() - requiredCnt; i < candidates.size(); ++i) {
            vSelected.emplace_back(candidates.at(i));
            MY_LOGD("[Selected] history frame historyFrameNo(%d)", candidates.at(i));
        }
    } else {
        MY_LOGD("No enough history frames");
        ret = false;
    }

    return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
bool
ZSDHDRSelector::
selectFromInflight(std::vector<FrameInfo>& vSelected,
                   int32_t const& requiredCnt,
                   std::vector<NSCam::ZslPolicy::InFlightInfo> const& inflightFrames) const
{
    bool ret = true;

    if (requiredCnt <= 0) {
        MY_LOGW("No need to select from inflight frames");
        return ret;
    }
    MY_LOGD("requiredCnt(%d)", requiredCnt);

    for (int i = inflightFrames.size() - requiredCnt; i < inflightFrames.size(); ++i) {
        auto& frameNo = inflightFrames.at(i).frameNo;
        auto& requestNo = inflightFrames.at(i).requestNo;

        FrameInfo selected;
        selected.frameNo = frameNo;
        selected.isChecked = false;
        vSelected.emplace_back(selected);
        MY_LOGD("[Selected] inflight frame frameNo(%d) requestNo(%d)",
                frameNo, requestNo);
    }

    return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRSelector::
select_NonZsl(SelectOutputParams& out,
              SelectInputParams const& in,
              ParsedZslRequestInfo const& parsedZslRequestInfo) const
{
    auto& zslRequest = in.zslRequest;
    MY_LOGD("run non-ZSL ZSDHDR (durationMs:%ld)", zslRequest.durationMs);

    // generate output (Non-ZSL)
    ZSDHDRReqSetting reqSetting;
    generateOut(out,
                parsedZslRequestInfo.zslFrameIdx,
                reqSetting,
                true);
}

/******************************************************************************
 *
 ******************************************************************************/
bool
ZSDHDRSelector::
select_Zsl(ZSDHDRSelector::SelectOutputParams& out,
           ZSDHDRSelector::SelectInputParams const& in,
           ParsedZslRequestInfo const& parsedZslRequestInfo) const
{
    auto const& zslRequest = in.zslRequest;
    auto const& historyFrames = in.historyFrames;
    auto const& inflightFrames = in.inflightFrames;

    auto requestCnt = zslRequest.frames.size();
    auto historyCnt = historyFrames.size();
    auto inflightCnt = inflightFrames.size();
    MY_LOGD("requestCnt(%d) historyCnt(%d) inflightCnt(%d)",
            requestCnt, historyCnt, inflightCnt);

    if (parsedZslRequestInfo.bIsFirstFrame
            && mvRecordHistoryFrame.size() == 0
            && mvRecordInflightFrame.size() == 0) {
        // decide mvReqSetting
        decideReqSetting(parsedZslRequestInfo);

        // check inflight -> history
        auto sizeToMove = mZSLInflightCnt - (MINT8)inflightCnt;
        if (sizeToMove > 0) {
            for (auto& reqSetting : mvReqSetting) {
                if (reqSetting.mCategory == Category::eZSL_Inflight) {
                    if (reqSetting.mHisInfIdx < sizeToMove) {
                        reqSetting.mCategory = Category::eZSL_History;
                        reqSetting.mHisInfIdx = mZSLHistoryCnt;
                        mZSLHistoryCnt++;
                    } else {
                        reqSetting.mHisInfIdx -= sizeToMove;
                    }
                }
            }
            mZSLInflightCnt -= sizeToMove;

            MY_LOGD("======reset mvReqSetting======");
            dumpReqSetting();
        }

        // select from history
        if (!selectFromHistory(mvRecordHistoryFrame,
                               mZSLHistoryCnt,
                               historyFrames,
                               zslRequest.zsdTimestampNs)) {
            MY_LOGW("selectFromHistory fail and trigger non-ZSL ZSDHDR!!!");
            return false;
        }

        // lock selected history frames
        for (auto& recordFrame : mvRecordHistoryFrame) {
            out.operations.emplace(
                    recordFrame.frameNo, SelectOutputParams::OpT::lock);
            recordFrame.isLocked = true;
        }

        // select from inflight
        if(!selectFromInflight(mvRecordInflightFrame, mZSLInflightCnt, inflightFrames)) {
            MY_LOGW("selectFromInFlight fail and trigger non-ZSL ZSDHDR!!!");
            return false;
        }
    }

    if (mvRecordHistoryFrame.size() > 0 || mvRecordInflightFrame.size() > 0) {
        // check mvRecordInflightFrame (inflight -> history)
        for (auto& recordFrame : mvRecordInflightFrame) {
            auto it = historyFrames.find(recordFrame.frameNo);
            if (it != historyFrames.end()) {
                // check validExpNum
                if (!recordFrame.isChecked) {
                    auto& halMetas = it->second.halMetas;
                    for (auto& halMeta : halMetas) {
                        auto streamId = halMeta.first->getStreamId();
                        if (streamId == eSTREAMID_META_PIPE_DYNAMIC_01) {
                            auto& pMeta = halMeta.second;

                            MINT32 validExpNum = -1;
                            if (!IMetadata::getEntry<MINT32>(
                                    pMeta,
                                    MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                                    validExpNum)) {
                                MY_LOGW("Can not get MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM!!!");
                            }

                            if (validExpNum != MTK_STAGGER_VALID_EXPOSURE_3) {
                                MY_LOGW("[Check Not Valid] "
                                        "inflight frame->history frame(%d) validExpNum(%d)",
                                        recordFrame.frameNo, validExpNum);
                            }

                            extractBlobOrder(recordFrame, *pMeta);
                            break;
                        }
                    }
                    recordFrame.isChecked = true;
                }

                // lock selected inflight -> history frames
                if (!recordFrame.isLocked) {
                    out.operations.emplace(
                            recordFrame.frameNo, SelectOutputParams::OpT::lock);
                    recordFrame.isLocked = true;
                }
            }
        }

        // generate output
        {
            auto const& reqSetting = mvReqSetting.at(parsedZslRequestInfo.frameIndex);
            if (reqSetting.mCategory == Category::eZSL_Inflight) {
                auto const& recordFrameNo =
                        mvRecordInflightFrame.at(reqSetting.mHisInfIdx).frameNo;
                auto it = historyFrames.find(recordFrameNo);
                if (it != historyFrames.end()) {
                    generateOut(out,
                                parsedZslRequestInfo.zslFrameIdx,
                                reqSetting);
                }
            } else {
                generateOut(out,
                            parsedZslRequestInfo.zslFrameIdx,
                            reqSetting);
            }
        }

        // clear
        if (mCounter == 0) {
            mvRecordHistoryFrame.clear();
            mvRecordInflightFrame.clear();
        }

    } else {
        MY_LOGD("no record frames and trigger non-ZSL ZSDHDR!!!");
        return false;
    }

    return true;
}

/******************************************************************************
 *
 ******************************************************************************/
ZSDHDRSelector::SelectOutputParams
ZSDHDRSelector::
select(SelectInputParams const& in) const
{
    auto& zslRequest = in.zslRequest;
    MY_LOGD("reqNo(%d) zsdTimestamp(%ld) select +",
            zslRequest.requestNo, zslRequest.zsdTimestampNs);

    ParsedZslRequestInfo parsedZslRequestInfo = parseZslRequestInfo(zslRequest);

    if (mMode != NonZsl_Mode) {
        int32_t debugP1Reconfig =
                ::property_get_int32("vendor.debug.stagger.expnum", -1);
        bool bIsValidExpNum =
                (parsedZslRequestInfo.status == MTK_FEATURE_VHDR_STATUS_3EXP)
                || (parsedZslRequestInfo.status == MTK_FEATURE_VHDR_STATUS_2EXP);
        if (!bIsValidExpNum || (debugP1Reconfig == 1)) {
            mMode = NonZsl_Mode;
        }
        MY_LOGD("debugP1Reconfig: %d, bIsValidExpNum: %d",
                debugP1Reconfig, bIsValidExpNum);
    }
    MY_LOGD("ZSDHDR mode is %d", mMode);

    bool ret = true;
    SelectOutputParams out;
    if (mMode != NonZsl_Mode) {
        ret = select_Zsl(out, in, parsedZslRequestInfo);
    }

    bool bIsTimeOut = zslRequest.durationMs >= ZslPolicy::kTimeoutMs;
    if (!ret || mMode == NonZsl_Mode || bIsTimeOut) {
        select_NonZsl(out, in, parsedZslRequestInfo);
        mvRecordHistoryFrame.clear();
        mvRecordInflightFrame.clear();
    }

    dump(out);

    MY_LOGD("reqNo(%d) select -", zslRequest.requestNo);

    return out;
}
/******************************************************************************
 *
 ******************************************************************************/
};  // namespace Custom
};  // namespace NSCam

/******************************************************************************
 *
 ******************************************************************************/
extern "C" NSCam::ZslPolicy::ISelector* create() {
    return new NSCam::Custom::ZSDHDRSelector();
}
