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

#define LOG_TAG "ZSDHDRPlugin"

#include "ZSDHDRImpl.h"

#include <stdlib.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include <cutils/compiler.h>
#include <utils/String8.h>

#include <memory>
#include <vector>

#include <mtkcam3/pipeline/hwnode/StreamId.h>

#include <mtkcam/drv/iopipe/SImager/ISImager.h>
#include <mtkcam/utils/std/Log.h>
#include <mtkcam/utils/std/ULog.h>
#include <mtkcam/utils/std/Format.h>
#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>
#include <mtkcam/utils/TuningUtils/FileDumpNamingRule.h>
#include <mtkcam/utils/TuningUtils/IScenarioRecorder.h>

using NSCam::TuningUtils::FILE_DUMP_NAMING_HINT;
using NSCam::TuningUtils::RAW_PORT_NULL;
using NSCam::TuningUtils::scenariorecorder::DecisionParam;
using NSCam::TuningUtils::scenariorecorder::IScenarioRecorder;
using NSCam::TuningUtils::scenariorecorder::DECISION_FEATURE;
using NSCam::TuningUtils::scenariorecorder::DebugSerialNumInfo;

/******************************************************************************
 *
 ******************************************************************************/
CAM_ULOG_DECLARE_MODULE_ID(MOD_LIB_ZSDHDR);

#define MY_LOGV(fmt, arg...)        CAM_ULOGMV("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_ULOGMD("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_ULOGMI("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_ULOGMW("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_ULOGME("(%d)[%s] " fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_ULOGM_ASSERT(0, "[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_ULOGM_FATAL("[%s] " fmt, __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)
//
#define ASSERT(cond, msg)           do { if (!(cond)) { printf("Failed: %s\n", msg); return; } }while(0)

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace NSPipelinePlugin{
/******************************************************************************
 *
 ******************************************************************************/
ZSDHDRSelector::
ZSDHDRSelector(std::vector<ZSDHDRReqSetting>& vReqSetting)
    : mvReqSetting(vReqSetting)
{
    init();
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
void
ZSDHDRSelector::
init() const
{
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

    dumpReqSetting();
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
dump(const SelectOutputParams& out) const
{
    auto& selected = out.selected;
    for (auto it : selected) {
        auto& zslFrameIdx = it.first;
        auto& zslFrameResult = it.second;
        MY_LOGD("[SelectOutputParams] zslFrameIdx(%lu) historyFrameNo(%d) clone(%d)",
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

        IMetadata::setEntry<MUINT8>(
                pMeta, MTK_MULTIFRAME_INDEX_ORDER_UNSURE, 1);
        zslFrameResult.additionalMetas.emplace(
                eSTREAMID_META_PIPE_DYNAMIC_01, additionalMeta);
    } else {
        zslFrameResult.frameNo = -1;

        IMetadata::setEntry<MUINT8>(pMeta,
                MTK_MULTIFRAME_INDEX_ORDER_UNSURE, 1);
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
    }

    out.selected.emplace(zslFrameIdx, zslFrameResult);
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
bool
ZSDHDRSelector::
selectFromInFlight(std::vector<FrameInfo>& vSelected,
                   int32_t const& requiredCnt,
                   std::vector<ZslPolicy::InFlightInfo> const& inflightFrames) const
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
            vSelected.push_back(candidates.at(i));
            MY_LOGD("[Selected] history frame historyFrameNo(%d)", candidates.at(i).frameNo);
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
ZSDHDRSelector::SelectOutputParams
ZSDHDRSelector::
select(SelectInputParams const& in) const
{
    CAM_ULOGM_APILIFE();

    auto& zslRequest = in.zslRequest;
    auto& historyFrames = in.historyFrames;
    auto& inflightFrames = in.inflightFrames;
    MY_LOGD("reqNo(%d) zsdTimestamp(%ld) select +",
            zslRequest.requestNo, zslRequest.zsdTimestampNs);

    auto requestCnt = zslRequest.frames.size();
    auto historyCnt = historyFrames.size();
    auto inflightCnt = inflightFrames.size();
    MY_LOGD("requestCnt(%zu) historyCnt(%zu) inflightCnt(%zu)",
            requestCnt, historyCnt, inflightCnt);

    bool ret = true;
    SelectOutputParams out;
    if (mvRecordHistoryFrame.size() == 0 && mvRecordInflightFrame.size() == 0) {
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
            ret = false;
            goto lbDone;
        }

        // lock selected history frames
        for (auto& recordFrame : mvRecordHistoryFrame) {
            out.operations.emplace(
                    recordFrame.frameNo, SelectOutputParams::OpT::lock);
            recordFrame.isLocked = true;
        }

        // select from inflight
        if(!selectFromInFlight(mvRecordInflightFrame, mZSLInflightCnt, inflightFrames)) {
            MY_LOGW("selectFromInFlight fail and trigger non-ZSL ZSDHDR!!!");
            ret = false;
            goto lbDone;
        }

        // generate output (Non-ZSL)
        for (auto i = 0; i < mvReqSetting.size(); ++i) {
            if (mvReqSetting.at(i).mCategory == Category::eNonZSL) {
                generateOut(out, i, mvReqSetting.at(i));
            }
        }
    }

    if (mvRecordHistoryFrame.size() > 0 || mvRecordInflightFrame.size() > 0) {
        // check mvRecordInflightFrame (inflight -> history)
        bool bKeepPending = false;
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
                            if (!IMetadata::getEntry<MINT32>(pMeta,
                                    MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                                    validExpNum)) {
                                MY_LOGW("Can not get MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM!!!");
                            }

                            if (validExpNum != MTK_STAGGER_VALID_EXPOSURE_3) {
                                MY_LOGW("No way to avoid invalid validExpNum "
                                        "for inflight frames currently!!!");
                            }

                            extractBlobOrder(recordFrame, *pMeta);
                            break;
                        }
                    }  // for-loop
                    recordFrame.isChecked = true;
                }

                // lock selected inflight -> history frames
                if (!recordFrame.isLocked) {
                    out.operations.emplace(
                            recordFrame.frameNo, SelectOutputParams::OpT::lock);
                    recordFrame.isLocked = true;
                }
            } else {
                bKeepPending = true;
                break;
            }
        }

        if (!bKeepPending) {
            // generate output (ZSL)
            for (auto i = 0; i < mvReqSetting.size(); ++i) {
                if (mvReqSetting.at(i).mCategory != Category::eNonZSL) {
                    generateOut(out, i, mvReqSetting.at(i));
                }
            }

            // clear
            mvRecordHistoryFrame.clear();
            mvRecordInflightFrame.clear();
        }
    }

lbDone:
    if (!ret || (zslRequest.durationMs) >= ZslPolicy::kTimeoutMs) {
        select_NonZsl(out, in);
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
void
ZSDHDRSelector::
select_NonZsl(SelectOutputParams& out,
              SelectInputParams const& in) const
{
    auto& zslRequest = in.zslRequest;
    MY_LOGD("run non-ZSL ZSDHDR (durationMs:%ld)", zslRequest.durationMs);

    // generate output (Non-ZSL)
    ZSDHDRReqSetting reqSetting;
    for (auto i = 0; i < mvReqSetting.size(); ++i) {
        generateOut(out, i, reqSetting, true);
    }
}

/******************************************************************************
 *
 ******************************************************************************/
REGISTER_PLUGIN_PROVIDER(MultiFrame, ZSDHDRPluginProviderImp);

/******************************************************************************
 *
 ******************************************************************************/
ZSDHDRPluginProviderImp::
ZSDHDRPluginProviderImp()
    : mHDRHalMode(MTK_HDR_FEATURE_HDR_HAL_MODE_OFF)
{
    CAM_ULOGM_APILIFE();
    mEnable = ::property_get_int32("vendor.debug.camera.zsdhdr.enable", -1);
    mMode = ::property_get_int32("vendor.debug.camera.zsdhdr.mode", 0);
    mAlgoType = ::property_get_int32("vendor.camera.zsdhdr.type", 0);  // 0: YuvDomainHDR, 1: RawDomainHDR
    mEnableLog = ::property_get_int32("vendor.debug.camera.zsdhdr.log", 0);
    mDumpBuffer = ::property_get_int32("vendor.debug.camera.zsdhdr.dump", 0);

    initRequestTable();

    mZsdBufferMaxNum = 0;
    for (auto const& [key, value] : mRequestTable) {
        if (value.size() > mZsdBufferMaxNum) {
            mZsdBufferMaxNum = value.size();
        }
    }
}

/******************************************************************************
 *
 ******************************************************************************/
ZSDHDRPluginProviderImp::
~ZSDHDRPluginProviderImp()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
const
ZSDHDRPluginProviderImp::Property&
ZSDHDRPluginProviderImp::
property()
{
    CAM_ULOGM_APILIFE();
    static Property prop;
    static bool inited;

    if (!inited) {
        prop.mName = "THIRD PARTY ZSDHDR";
        prop.mFeatures = TP_FEATURE_ZSDHDR;
        prop.mZsdBufferMaxNum = mZsdBufferMaxNum; // maximum frames requirement
        prop.mThumbnailTiming = eTiming_P2;
        prop.mPriority = ePriority_Highest;
        prop.mBoost = eBoost_CPU;
        inited = true;
    }

    return prop;
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
ZSDHDRPluginProviderImp::
negotiate(Selection& sel)
{
    CAM_ULOGM_APILIFE();

    // check meta
    IMetadata* pAppInMeta = nullptr;
    IMetadata* pHalInMeta = nullptr;
    bool isAppMeta = sel.mIMetadataApp.getControl() != NULL;
    bool isHalMeta = sel.mIMetadataHal.getControl() != NULL;
    if (isAppMeta) {
        pAppInMeta = sel.mIMetadataApp.getControl().get();
    }
    if (isHalMeta) {
        pHalInMeta = sel.mIMetadataHal.getControl().get();
    }

    // ZSDHDR is force off
    if (mEnable == 0) {
        MY_LOGD("force ZSDHDR off");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta, "force ZSDHDR off");
        }
        return BAD_VALUE;
    }

    // ZSDHDR is off on low RAM devices
    bool isLowRamDevice = ::property_get_bool("ro.config.low_ram", false);
    if (isLowRamDevice) {
        MY_LOGI("low RAM device, force ZSDHDR off");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                    "low RAM device, force ZSDHDR off");
        }
        return BAD_VALUE;
    }

    // ZSDHDR on/off based on HDR scenario
    if (isAppMeta) {
        MUINT8 sceneMode = 0;
        bool ret = IMetadata::getEntry<MUINT8>(
                pAppInMeta, MTK_CONTROL_SCENE_MODE, sceneMode);

        if (CC_UNLIKELY(!ret)) {
            MY_LOGE("cannot get MTK_CONTROL_SCENE_MODE");
            if (isHalMeta) {
                writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                        "cannot get MTK_CONTROL_SCENE_MODE");
            }
            return BAD_VALUE;
        }

        if (sceneMode != MTK_CONTROL_SCENE_MODE_HDR) {
            if (mEnable == 1) {
                MY_LOGD("force ZSDHDR on");
                if (isHalMeta) {
                    writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                            "force ZSDHDR on");
                }
            } else {
                MY_LOGD("no need to execute ZSDHDR");
                if (isHalMeta) {
                    writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                            "no need to execute ZSDHDR");
                }
                return BAD_VALUE;
            }
        }
    } else {
        MY_LOGE("cannot get App Meta");
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                                    "cannot get App Meta");
        }
        return BAD_VALUE;
    }

    // ZSDHDR on/off based on HDR mode
    auto const& mHDRHalMode = sel.mState.mHDRHalMode;
    if (mHDRHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_MSTREAM_CAPTURE) {
        MY_LOGD("Mstream HDR is on");
    } else if (mHDRHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP) {
        MY_LOGD("Stagger_2EXP HDR is on");
    } else if (mHDRHalMode == MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP) {
        MY_LOGD("Stagger_3EXP HDR is on");
    } else {
        MY_LOGD("ZSDHDR is off due to no supported VHDR is on, "
                "mHDRHalMode: %d", mHDRHalMode);
        if (isHalMeta) {
            writeFeatureDecisionLog(sel.mSensorId, pHalInMeta,
                    "ZSDHDR is off due to no supported VHDR is on");
        }
        return BAD_VALUE;
    }

    // decide ZSDHDR mode
    if (mMode != NonZsl_Mode) {
        int32_t debugP1Reconfig =
                ::property_get_int32("vendor.debug.stagger.expnum", -1);
        bool bIsValidZsl = sel.mState.mZslPoolReady && sel.mState.mZslRequest;
        bool bIsValidExpNum =
                (sel.mState.mValidExpNum == MTK_STAGGER_VALID_EXPOSURE_3)
                || (sel.mState.mValidExpNum == MTK_STAGGER_VALID_EXPOSURE_2);
        if (!bIsValidZsl || !bIsValidExpNum || (debugP1Reconfig == 1)) {
            mMode = NonZsl_Mode;
        }
        MY_LOGD("(debugP1Reconfig: %d, bIsValidZsl: %d, bIsValidExpNum: %d)",
                debugP1Reconfig, bIsValidZsl, bIsValidExpNum);
    }
    MY_LOGD("ZSDHDR mode is %d", mMode);

    if (sel.mRequestIndex == 0) {
        if (mMode == NonZsl_Mode) {
            sel.mDecision.mZslEnabled = false;
        } else {
            sel.mDecision.mZslEnabled = true;
            mSelector =
                    std::make_shared<ZSDHDRSelector>(
                    mRequestTable[std::make_tuple(mMode, mHDRHalMode)]);
            sel.mDecision.pZslSelector = mSelector;
        }
    }

    if (mMode == NonZsl_Mode) {
        sel.mRequestCount = 7;  // hard code
    } else {
        sel.mRequestCount =
                mRequestTable[std::make_tuple(mMode, mHDRHalMode)].size();
    }

    // ZSDHDR based on mAlgoType
    int inputFormat = eImgFmt_NV21;
    int outputFormat = eImgFmt_NV21;
    if (mAlgoType == RawDomainHDR) {
        inputFormat = eImgFmt_BAYER10;
        outputFormat = eImgFmt_BAYER10;
    }

    sel.mIBufferFull.setRequired(MTRUE)
                    .addAcceptedFormat(inputFormat)
                    .addAcceptedSize(eImgSize_Full);
    sel.mIMetadataDynamic.setRequired(MTRUE);
    sel.mIMetadataApp.setRequired(MTRUE);
    sel.mIMetadataHal.setRequired(MTRUE);

    // Only main frame has output buffer
    if (sel.mRequestIndex == 0) {
        sel.mOBufferFull.setRequired(MTRUE)
                        .addAcceptedFormat(outputFormat)
                        .addAcceptedSize(eImgSize_Full);
        if (mAlgoType == RawDomainHDR) {
            sel.mOBufferFull.setAlignment(64, 0);
        }
        sel.mOMetadataApp.setRequired(MTRUE);
        sel.mOMetadataHal.setRequired(MTRUE);
    } else {
        sel.mOBufferFull.setRequired(MFALSE);
        sel.mOMetadataApp.setRequired(MFALSE);
        sel.mOMetadataHal.setRequired(MFALSE);
    }

    // set additional metadata
    if (isAppMeta && isHalMeta) {
        MetadataPtr pAppAdditional = std::make_shared<IMetadata>();
        MetadataPtr pHalAddtional = std::make_shared<IMetadata>();

        IMetadata* pAppMeta = pAppAdditional.get();
        IMetadata* pHalMeta = pHalAddtional.get();

        if (mMode == NonZsl_Mode) {
            // do seamless switch for non-zsl ZSDHDR
            IMetadata::setEntry<MINT32>(pHalMeta,
                    MTK_3A_FEATURE_AE_TARGET_MODE,
                    MTK_3A_FEATURE_AE_TARGET_MODE_NORMAL);
            IMetadata::setEntry<MINT32>(pHalMeta,
                    MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM,
                    MTK_STAGGER_VALID_EXPOSURE_1);
            IMetadata::setEntry<MINT32>(pHalMeta,
                    MTK_3A_ISP_FUS_NUM,
                    MTK_3A_ISP_FUS_NUM_1);
        }

        sel.mIMetadataApp.setAddtional(pAppAdditional);
        sel.mIMetadataHal.setAddtional(pHalAddtional);
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRPluginProviderImp::
init()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
MERROR
ZSDHDRPluginProviderImp::
process(RequestPtr pRequest,
        RequestCallbackPtr pCallback)
{
    CAM_ULOGM_APILIFE();

    // set thread's name
    ::prctl(PR_SET_NAME, "ZSDHDR Plugin", 0, 0, 0);

    // restore callback function for abort API
    if (pCallback != nullptr) {
        mpCallback = pCallback;
    }

    // check buffer and meta
    bool isIBufferFull = pRequest->mIBufferFull != nullptr;
    bool isOBufferFull = pRequest->mOBufferFull != nullptr;
    bool isIMetaHal = pRequest->mIMetadataHal != nullptr;
    bool isIMetaDynamic = pRequest->mIMetadataDynamic != nullptr;

    // get hal meta & decide bufferIndex
    int bufferIndex = 0;
    if (isIMetaHal) {
        IMetadata* pIMetaHal = pRequest->mIMetadataHal->acquire();

        // for debug
        MINT32 aeTargetMode = -1, validExpNum = -1, ispFusNum = -1;
        IMetadata::getEntry<MINT32>(pIMetaHal, MTK_3A_FEATURE_AE_TARGET_MODE, aeTargetMode);
        IMetadata::getEntry<MINT32>(pIMetaHal, MTK_3A_FEATURE_AE_VALID_EXPOSURE_NUM, validExpNum);
        IMetadata::getEntry<MINT32>(pIMetaHal, MTK_3A_ISP_FUS_NUM, ispFusNum);

        // query bufferIndex
        if (!IMetadata::getEntry<MINT32>(
                pIMetaHal, MTK_FEATURE_VALID_IMAGE_INDEX, bufferIndex)) {
            IMetadata::IEntry entry = pIMetaHal->entryFor(MTK_STAGGER_BLOB_IMGO_ORDER);
            MUINT8 entrySize = entry.count();
            for (int i = 0; i < entrySize; ++i) {
                auto tmp = entry.itemAt(i, Type2Type<MUINT8>());
                if (tmp == MTK_STAGGER_IMGO_NE) {
                    bufferIndex = i;
                    break;
                }
            }
        }

        MY_LOGD("bufferIndex is %d (mMode: %d, mAlgoType: %d, "
                "aeTargetMode: %d, validExpNum: %d, ispFusNum: %d)",
                bufferIndex, mMode, mAlgoType,
                aeTargetMode, validExpNum, ispFusNum);
    }

    // get input buffer & query colorOrder
    int colorOrder = -1;
    IImageBuffer* pIImgBufferToCopy = nullptr;
    if (isIBufferFull) {
        auto DumpBufferInfo = [&](IImageBuffer* pIImgBuffer, int idx = 0) {
            MY_LOGD("[IN] Full image VA: 0x%p "
                    "size:(%d,%d) fmt: 0x%x planecount: %zu colorOrder: %d",
                    (void*)pIImgBuffer->getBufVA(0),
                    pIImgBuffer->getImgSize().w, pIImgBuffer->getImgSize().h,
                    pIImgBuffer->getImgFormat(),
                    pIImgBuffer->getPlaneCount(),
                    pIImgBuffer->getColorArrangement());

            if (mDumpBuffer) {
                // dump input buffer
                const char* pUserString =
                        String8::format("zsdhdr-in-%d-%d", pRequest->mRequestIndex, idx).c_str();
                IMetadata* pHalMeta = pRequest->mIMetadataHal->acquire();
                dumpBuffer(pHalMeta, pRequest->mSensorId, pIImgBuffer, pUserString);
            }
        };

        if (mAlgoType == YuvDomainHDR) {
            IImageBuffer* pIImgBuffer = pRequest->mIBufferFull->acquire();
            DumpBufferInfo(pIImgBuffer);
            pIImgBufferToCopy = pIImgBuffer;
        } else {
            std::vector<IImageBuffer*> vpIImgBuffer = pRequest->mIBufferFull->acquireAll();
            for (int i = 0; i < vpIImgBuffer.size(); ++i) {
                auto pIImgBuffer = vpIImgBuffer.at(i);
                DumpBufferInfo(pIImgBuffer, i);
            }
            pIImgBufferToCopy = vpIImgBuffer.at(bufferIndex);
        }
        colorOrder = pIImgBufferToCopy->getColorArrangement();
    }

    // get output buffer & copy input buffer to output buffer
    if (isOBufferFull && pIImgBufferToCopy) {
        IImageBuffer* pOImgBuffer = pRequest->mOBufferFull->acquire();
        MY_LOGD("[OUT] Full image VA: 0x%p "
                "size:(%d,%d) fmt: 0x%x planecount: %zu colorOrder: %d",
                (void*)pOImgBuffer->getBufVA(0),
                pOImgBuffer->getImgSize().w, pOImgBuffer->getImgSize().h,
                pOImgBuffer->getImgFormat(),
                pOImgBuffer->getPlaneCount(),
                pOImgBuffer->getColorArrangement());

        // copy input buffer to output buffer
        if (mAlgoType == YuvDomainHDR) {
            NSIoPipe::NSSImager::ISImager *pISImager = nullptr;
            pISImager = NSIoPipe::NSSImager::ISImager::createInstance(pIImgBufferToCopy);
            if (CC_LIKELY(pISImager)) {
                if (!pISImager->setTargetImgBuffer(pOImgBuffer)) {
                    MY_LOGW("SImager::setTargetImgBuffer fail");
                }
                if (!pISImager->execute()) {
                    MY_LOGW("SImager::execute fail");
                }
                pISImager->destroyInstance();
            } else {
                MY_LOGW("ISImager::createInstance fail");
            }
        } else {
            for (size_t i = 0; i < pOImgBuffer->getPlaneCount(); ++i) {
                size_t planeBufSize = pOImgBuffer->getBufSizeInBytes(i);
                MUINT8* srcPtr = (MUINT8*)pIImgBufferToCopy->getBufVA(i);
                void* dstPtr = (void*)pOImgBuffer->getBufVA(i);
                memcpy(dstPtr, srcPtr, planeBufSize);
            }

            // set color arrangement
            pOImgBuffer->setColorArrangement(colorOrder);
            MY_LOGD("set colorOrder to %d", colorOrder);
        }

        if (mDumpBuffer) {
            // dump otuput buffer
            const char* pUserString =
                    String8::format("zsdhdr-out-%d", pRequest->mRequestIndex).c_str();
            IMetadata* pHalMeta = pRequest->mIMetadataHal->acquire();
            dumpBuffer(pHalMeta, pRequest->mSensorId, pOImgBuffer, pUserString);
        }
    }

    mvpRequest.push_back(pRequest);
    MY_LOGD("collect request(%d/%d)", pRequest->mRequestIndex+1, pRequest->mRequestCount);

    if (pRequest->mRequestIndex == pRequest->mRequestCount - 1) {
        MY_LOGD("have collected all requests");

        // perform callback
        for (auto& req : mvpRequest) {
            MY_LOGD("callback request(%d/%d) %p",
                    req->mRequestIndex+1, req->mRequestCount, pCallback.get());
            if (pCallback != nullptr) {
                pCallback->onCompleted(req, 0);
            }
        }

        // clear data
        mvpRequest.clear();
    }

    return OK;
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRPluginProviderImp::
abort(std::vector<RequestPtr>& pRequests)
{
    CAM_ULOGM_APILIFE();

    bool isAbort = false;

    if (mpCallback == nullptr) {
        MY_LOGA("callbackptr is null");
    }

    for (auto& req: pRequests) {
        isAbort = false;

        for (std::vector<RequestPtr>::iterator it = mvpRequest.begin();
             it != mvpRequest.end(); ++it) {
            if ((*it) == req) {
                mvpRequest.erase(it);
                mpCallback->onAborted(req);
                isAbort = true;
                break;
            }
        }

        if (!isAbort) {
            MY_LOGW("desire abort request[%d] is not found", req->mRequestIndex);
        }
    }

    if (!mvpRequest.empty()) {
        MY_LOGW("abort() does not clean all the requests");
    } else {
        MY_LOGW("abort() cleans all the requests");
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRPluginProviderImp::
uninit()
{
    CAM_ULOGM_APILIFE();
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRPluginProviderImp::
dumpBuffer(const IMetadata* pHalMeta,
           const MINT32 sensorId,
           IImageBuffer* pIImgBuffer,
           const char* pUserString)
{
    char fileName[256] = "";

    FILE_DUMP_NAMING_HINT hint{};
    memset(&hint, 0, sizeof(FILE_DUMP_NAMING_HINT));

    // extract hint from metadata
    extract(&hint, pHalMeta);

    // extract hint from sensor
    extract_by_SensorOpenId(&hint, sensorId);

    // extract hint from buffer
    extract(&hint, pIImgBuffer);

    genFileName_RAW(fileName, sizeof(fileName), &hint, RAW_PORT_NULL, pUserString);

    pIImgBuffer->saveToFile(fileName);
}

/******************************************************************************
 *
 ******************************************************************************/
void
ZSDHDRPluginProviderImp::
writeFeatureDecisionLog(int32_t sensorId,
                        IMetadata* pHalInMeta,
                        const char* log)
{
    if (IScenarioRecorder::getInstance()->isScenarioRecorderOn()
        && log) {
        int32_t requestNum = -1;
        if (!IMetadata::getEntry<MINT32>(pHalInMeta,
                                         MTK_PIPELINE_REQUEST_NUMBER, requestNum)) {
            MY_LOGW("Get MTK_PIPELINE_REQUEST_NUMBER fail");
        }

        int32_t uniquekey = 0;
        if (!IMetadata::getEntry<MINT32>(pHalInMeta,
                                         MTK_PIPELINE_UNIQUE_KEY, uniquekey)) {
            MY_LOGW("Get MTK_PIPELINE_UNIQUE_KEY fail");
        }

        DecisionParam decParam;
        DebugSerialNumInfo& dbgNumInfo = decParam.dbgNumInfo;
        dbgNumInfo.uniquekey = uniquekey;
        dbgNumInfo.reqNum = requestNum;
        decParam.decisionType = DECISION_FEATURE;
        decParam.moduleId = NSCam::Utils::ULog::MOD_LIB_HDR;
        decParam.sensorId = sensorId;
        decParam.isCapture = true;
        IScenarioRecorder::getInstance()->submitDecisionRecord(decParam, log);
    }
}

};  // namespace NSPipelinePlugin
};  // namespace NSCam
