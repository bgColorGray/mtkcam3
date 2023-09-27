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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_DEF_ZSLPOLICY_TYPES_H_
#define _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_DEF_ZSLPOLICY_TYPES_H_

#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include <mtkcam/utils/metadata/IMetadata.h>

#include <inttypes.h>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <unordered_map>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam::ZslPolicy {  // TODO: rename ns ?

enum Criteria : uint32_t
{
    // selected buffers need to meet the criteria.
    // bit 0~15 image quality. (check from metadata)
    None                            = 0x0,
    AFStable                        = 0x1 << 0,
    AEStable                        = 0x1 << 1,  // by checking MTK_CONTROL_AE_STATE
    AESoftStable                    = 0x1 << 2,  // by checking MTK_3A_AE_ZSL_STABLE, less strict

    // bit 16~23: zsl behavior.
    Behavior_MASK                   = 0x00FF0000,
    ZeroShutterDelay                = ( 0x1 << 0 ) << 16,
    ContinuousFrame                 = ( 0x1 << 1 ) << 16,

    // bit 24~31: need additional information. (provided by user)
    Customize_MASK                  = 0xFF000000,
    UserSelect                      = ( 0x1 << 1 ) << 24,  //TODO:
};

/**
 *  Feature Decision (TODO: replace with ISelector.)
 */
constexpr int64_t kDefaultZslTimeoutFor3AStable = 1000;
struct ZslPluginParams
{
    uint32_t                        mPolicy = None;  // TO-BE: criteria
    int64_t                         mTimeouts = kDefaultZslTimeoutFor3AStable;  // ms

    /**
     * Batch Selection
     *
     * The selection is done in the 1st-req (index = 0), with #total_frames of buffers.
     * Produce #frames ppl_frame, and keep the remaining buffers for the following requests. (index > 0)
     *
     * Batched zsl capture requests should be submitted as bursts. (in sequence, no preview in between)
     * For non-burst requests: index = -1, #total_frames = #frames
     *
     * @param burstIndex: indice of this request of the burst requests.
     * @param totalFrameSize: #total_frames needed for the burst requests.
     *                          meaningful only when burstIndex = 0.
     */
    int32_t                         burstIndex = -1;
    int32_t                         totalFrameSize = 0;
};

static inline std::string toString(uint32_t const criteria)
{
    if (criteria == 0) return "{ None }";
    std::string s("{ ");
    if (criteria & AFStable)         { s += "AFStable ";         }
    if (criteria & AEStable)         { s += "AEStable ";         }
    if (criteria & AESoftStable)     { s += "AESoftStable ";     }
    if (criteria & ZeroShutterDelay) { s += "ZeroShutterDelay "; }
    if (criteria & ContinuousFrame)  { s += "ContinuousFrame ";  }
    if (criteria & UserSelect)       { s += "UserSelect ";       }
    s += "}";
    return s;
}

static inline std::string toString(ZslPluginParams const& o)
{
    std::ostringstream oss;
    oss << "{"
        << " criteria=" << toString(o.mPolicy)
        << " timeouts=" << o.mTimeouts
        << " index/count=" << o.burstIndex << "/" << o.totalFrameSize
        << " }";
    return oss.str();
}

static inline std::string toString(std::shared_ptr<ZslPluginParams>const& p)
{
    return p ? toString(*p) : "X";
}

/**
 * RequiredStreams: zsl frame required streams. (from RequestResultParams)
 *
 * @param halImages: required hal image streams.
 * @param halMetas: required hal meta streams.
 * @param appMetas: required app meta streams.
 *  note. std::includes can be used to check if historyFrame has the subset.
 */
struct RequiredStreams
{
    std::set<v3::StreamId_T> halImages;
    std::set<v3::StreamId_T> halMetas;
    std::set<v3::StreamId_T> appMetas;
};

/**
 * InFlightInfo: information of the inflight frame (historyFrame to-be)
 *
 * @param frameNo: frameNo.
 * @param requestNo: requestNo.
 * @param
 */
struct InFlightInfo
{
    uint32_t                                frameNo = 0;
    uint32_t                                requestNo = 0;
    // FIXME: duplicate with TrackFrameResultParams
    std::shared_ptr<RequiredStreams>        pContains;
};

static inline std::string toString(RequiredStreams const& o)
{
    std::ostringstream oss;
    oss << std::hex << "{ halImages{";
    for (auto streamId : o.halImages) {
        oss << streamId << " ";
    }
    oss << "} halMetas{";
    for (auto streamId : o.halMetas) {
        oss << streamId << " ";
    }
    oss << "} appMetas{";
    for (auto streamId : o.appMetas) {
        oss << streamId << " ";
    }
    oss << "} }";
    return oss.str();
}

/**
 * Priority of ISelector: Plugin > Custom > Default.
 *
 * Custom Selector should implement below for instantiation.
 * extern "C" ISelector* create();
 */
constexpr int64_t kTimeoutMs = 3000;
class ISelector
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definition.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    using ZslFrameIdxT = size_t;  // index of the frames in one request.
    using HistoryFrameNoT = uint32_t;
    using SensorModeMap = std::unordered_map<uint32_t, uint32_t>;
            // key: sensor id, value: sensor mode

public:     ////    SelectInputParams
    /**
     * ZslFrameInfo: infos of the zsl frame.
     *
     * @param requiredStreams: output from evaluateRequest. (RequestResultParams)
     * @param ctrlMetas: app/hal control metas from evaluateRequest.
     *  note. AppMeta_Ctrl * 1 + HalMeta_Ctrl * N ; N = #numbers of P1
     * @param pRequiredSensorMode
     */
    struct ZslFrameInfo
    {
        std::shared_ptr<RequiredStreams>        pRequired;
        std::map<v3::IMetaStreamInfo const*, IMetadata const*>
                                                ctrlMetas;
        std::shared_ptr<SensorModeMap>          pRequiredSensorModes;
    };

    /**
     * ZslRequestInfo: information of the zsl capture request.
     *
     * @param requestNo
     * @param frames: information of the zsl frames.
     *  After selection the frame will be removed.
     * @param durationMs: duration since the reqeust is received.
     *  - durationMs == 0 indicates the 1st selection.
     *  - if (durationMs > kTimeoutMs) final selection must be made. (#select == #frames)
     * @param zsdTimestampNs: estimated timestamp of the zsd buffer.
     *  zsdTimestamp = currentTime - zsdOffset,
     *  where zsdOffset is determined by "ro.vendor.camera3.zsl.default/project"
     * @param suggestFrameSync: suggest selecting frameSync history frames.
     *  ref. check halMeta; tag=MTK_FRAMESYNC_RESULT; desired value=MTK_FRAMESYNC_RESULT_PASS;
     */
    struct ZslRequestInfo
    {
        uint32_t                                requestNo = -1;
        std::map<ZslFrameIdxT, ZslFrameInfo>    frames;
        int64_t                                 durationMs = 0;
        int64_t                                 zsdTimestampNs = 0;
        bool                                    suggestFrameSync = false;
    };

    /**
     * HistoryFrame: information of one history frame.
     *
     * @param frameNo: history frameNo.
     * @param requestNo: history requestNo.
     * @param sensorTimestampNs: sensor timestamp of the hisroy buffer.
     * @param halImages: [streamInfo] of the history hal images.
     * @param halMetas: [streamInfo & IMetadata] of the history hal metas.
     * @param appMetas: [streamInfo & IMetadata] of the history app metas.
     *  ref. IStreamInfo::getStreamId()
     * @param locked: if the frame is locked or not.
     *  ref. SelectOutputParams.[lock/unlock]
     */
    struct HistoryFrame
    {
        uint32_t                                frameNo = 0;
        uint32_t                                requestNo = 0;
        int64_t                                 sensorTimestampNs = 0;
        std::set<v3::IImageStreamInfo const*>   halImages;
        std::map<v3::IMetaStreamInfo const*, IMetadata const*>
                                                halMetas;
        std::map<v3::IMetaStreamInfo const*, IMetadata const*>
                                                appMetas;
        bool                                    locked = false;
    };

    struct SelectInputParams
    {
        /**
         * Information of the zsl capture request.
         */
        ZslRequestInfo zslRequest;

        /**
         * Information of history frames.
         */
        std::map<HistoryFrameNoT, HistoryFrame> historyFrames;

        /**
         * Information of frames that will turn into historyFrames after inflighting.
         *
         * Might be useful for selection.
         *
         * Eg. <zsl + non-zsl>
         *  If the request contains both zsl + non-zsl frames,
         *  and requires their continuity (no gap between zsl + non-zsl frames).
         * - step0. make sure historyFrames will be enough for zslFrames.
         *  (#historyFrames + #inflightFrames <= #zslFrames)
         * - step1. submit non-zsl frames by setting the latter frames as -1.
         *  (ref. SelectOutputParams.selected; latter = larger ZslFrameIdxT)
         *  And pending to wait until last inflight frame are ready.
         *  (last inflight frame will be the frame before non-zsl frames.)
         * - step2. select zsl frames.
         *
         * note.
         *  For the request contains both zsl + non-zsl frames,
         *  "every" frame should add MTK_MULTIFRAME_INDEX_ORDER_UNSURE=1 in its hal meta.
         *  (ref. ZslFrameResult.additionalMetas)
         */
        std::vector<InFlightInfo> inflightFrames;
    };


public:     ////    SelectOutputParams
    struct ZslFrameResult
    {
        /**
         * Selected frameNo
         */
        HistoryFrameNoT                         frameNo = 0;

        /**
         * Clone historyFrame to share with others.
         * - ImageSB contain same ImageBufferHeap (shallow copy)
         * - MetaSB duplicate whole instance (deep copy)
         */
        bool                                    clone = false;

        /**
         * Additional metadata to append to this frame.  (Optional)
         *
         * Specified StreamId to append additional metas.
         *
         * For zsl flow, set p1out streams. (eg. eSTREAMID_META_[APP/PIPE]_DYNAMIC_XX)
         * For non-zsl flow, set ctrl streams. (eg. eSTREAMID_META_[APP/PIPE]_CONTROL)
         */
        std::map<v3::StreamId_T, std::shared_ptr<IMetadata>>
                                                additionalMetas;
    };

    struct SelectOutputParams
    {
        /**
         * Result of the selection.
         *  FrameNos of the historyFrames to be selected,
         *  which should be contained in historyFrames or -1.
         *  -1 indicates non-zsl flow. (preview fps will drop)
         *  the selected frameNos should be unique. (except -1)
         *
         * The selected HistoryFrame should contain all of the required streams of ZslFrameInfo.
         * The frames should be selected in sequence of ZslRequestInfo::frames.
         *
         * Submit #select frames.
         *  If (#frames - #select > 0), the rest will be pending.
         *  If pending, select() will be triggered again in a while. (by next app request)
         *
         * Warning:
         *  In most cases, FrameIdx should be selected in order.
         *  If not,
         *    FrameCount/Index would be unexpected to P2_CaptureNode.
         *    Instead of aborting, they should support reorder.
         */
        std::map<ZslFrameIdxT, ZslFrameResult> selected;

        /**
         * Operations to do on historyFrames.  (Optional)
         *
         * If historyFrame does not exist or has been selected, the operation will be ignore.
         *
         * @param lock
         * @param unlock
         *
         * HistoryFrames can be locked for later selection; unlock if not needed.
         *
         * After locking, the frame
         *  - will not be released until selected/ unlocked/ flush.
         *  - is only visible to the selector with the same name in later select().
         *      ref. getName()
         *
         * Eg. both A & B implement ISelector.
         *  A's 1st select(): historyFrames: F41, F42, F43, F44, F45.
         *  A locked F42. And after a while ...
         *  B's 1st select(): historyFrames: F43, F44, F45, F46.
         *  (F41 is released; F42 is only visible to A)
         *  A's 2nd select(): historyFrames: F42, F44, F45, F46, F47.
         *
         * @param release
         *
         * Actively released HistoryFrames.
         *
         * Otherwise it is operated by FBM to release the earliest historyFrame.
         * The whole frame will be released. (images + metas)
         */
        enum class OpT
        {
            lock,
            unlock,
            release,
        };
        std::map<HistoryFrameNoT, OpT> operations;

    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    virtual         ~ISelector() = default;
    virtual auto    getName() const -> char const*;

    /**
     * Strategy to select historyFrames for zsl request.
     *
     * @param[in]: information for selection.
     * @return: results of selection.
     */
    virtual auto    select(SelectInputParams const& in) const -> SelectOutputParams   = 0;

};

static inline std::string toString(ISelector::ZslFrameInfo const& o)
{
    std::ostringstream oss;
    oss << "{"
        << " required=" << (o.pRequired ? toString(*o.pRequired) : "X")
        << " ctrlMetas#" << o.ctrlMetas.size()
        << "}";
    return oss.str();
}

static std::string toString(ISelector::ZslRequestInfo const& o)
{
    auto toString_Frames __unused = [](auto const& v) {
        std::ostringstream oss;
        for (auto const& pr : v) {
            oss << "\n" << std::to_string(pr.first)
                << "-" << toString(pr.second) << " ";
        }
        oss << "\n";
        return oss.str();
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::ostringstream oss;
    oss << "{"
        << " requestNo=" << o.requestNo
        ///<< " frames=" << toString_Frames(o.frames)
        << " frames#" << o.frames.size()
        << " durationMs=" << o.durationMs
        << " zsdTimestampNs=" << o.zsdTimestampNs
        << " suggestFrameSync=" << o.suggestFrameSync
        << "}";
    return oss.str();
}

static inline std::string toString(ISelector::HistoryFrame const& o)
{
    std::ostringstream oss;
    oss << "[F" << o.frameNo
        << " R" << o.requestNo
        << " Ts" << o.sensorTimestampNs
        << " halImage#" << o.halImages.size()
        << " halMeta#" << o.halMetas.size()
        << " appMeta#" << o.appMetas.size()
        << " locked=" << o.locked
        << "]";
    return oss.str();
}

__unused static std::string toString(ISelector::SelectInputParams const& o)
{
    auto toString_HistoryFrames = [](auto const& v) {
        std::ostringstream oss;
        for (auto const& pr : v) {
            oss << "\n" << std::to_string(pr.first)
                << "-" << toString(pr.second) << " ";
        }
        return oss.str();
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::ostringstream oss;
    oss << "{"
        << " zslRequest=" << toString(o.zslRequest)
        << " historyFrames=" << toString_HistoryFrames(o.historyFrames)
        << "}";
    return oss.str();
}

static inline std::string toString(ISelector::ZslFrameResult const& o)
{
    std::ostringstream oss;
    oss << "[F" << (int32_t)o.frameNo
        << " addMeta(#" << o.additionalMetas.size() << ")"
        << "]";
    return oss.str();
}

__unused static std::string toString(ISelector::SelectOutputParams const& o)
{
    auto toString_sel = [](auto const& v) {
        std::ostringstream oss;
        oss << "{ ";
        for (auto& pr : v) {
            oss << pr.first << "-" << toString(pr.second) << " ";
        }
        oss << "}";
        return oss.str();
    };
    //
    auto toString_op = [](auto const& ops) {
        std::ostringstream oss;
        oss << "{ ";
        for (auto& pr  : ops) {
            oss << "F" << pr.first << "(" << static_cast<int>(pr.second) << ") ";
        }
        oss << "}";
        return oss.str();
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::ostringstream oss;
    oss << "{"
        << " selected=" << toString_sel(o.selected)
        << " ops=" << toString_op(o.operations)
        << "}";
    return oss.str();
}

static inline std::string toString(std::shared_ptr<ISelector>const& p)
{
    return p ? std::string(p->getName()) : "X";
}


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam::ZslPolicy
#endif//_MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_DEF_ZSLPOLICY_TYPES_H_
