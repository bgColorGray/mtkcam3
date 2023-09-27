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

#ifndef  _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_CUSTOM_ZSDHDRSELECTOR_H_
#define  _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_CUSTOM_ZSDHDRSELECTOR_H_

#include <stdlib.h>

#include <vector>

#include <mtkcam3/def/zslPolicy/types.h>

namespace NSCam {
namespace Custom {
/******************************************************************************
 *
 ******************************************************************************/
struct ParsedZslRequestInfo
{
    MUINT32 requestNo = 0;
    MINT32 frameIndex = -1;
    MINT32 frameCount = -1;
    size_t zslFrameIdx;
    MBOOL bIsFirstFrame = MFALSE;
    MBOOL bIsLastFrame = MFALSE;

    MINT32 status = -1;

    std::shared_ptr<NSCam::ZslPolicy::RequiredStreams> pRequiredStreams = nullptr;
};

class ZSDHDRSelector : public NSCam::ZslPolicy::ISelector {
  public:
    using NSCam::ZslPolicy::ISelector::SelectOutputParams;
    using NSCam::ZslPolicy::ISelector::SelectInputParams;

    ZSDHDRSelector();

    virtual const char* getName() const override;

    virtual SelectOutputParams select(SelectInputParams const& in) const override;

  private:
    struct FrameInfo {
        uint32_t frameNo;
        uint8_t NEIdx;
        uint8_t MEIdx;
        uint8_t SEIdx;
        bool isChecked;
        bool isLocked;
        bool isZSD;

        FrameInfo() : frameNo(0)
                    , NEIdx(0)
                    , MEIdx(0)
                    , SEIdx(0)
                    , isChecked(false)
                    , isLocked(false)
                    , isZSD(false) {}
    };

    enum class Category {
        eZSL_History,
        eZSL_Inflight,
        eNonZSL
    };

    struct ZSDHDRReqSetting {
        MINT8 mExp;
        MINT8 mOrder;
        Category mCategory;
        MBOOL mbClone;
        MINT8 mHisInfIdx;

        ZSDHDRReqSetting()
                : mExp(-1)
                , mOrder(-1)
                , mCategory(Category::eNonZSL)
                , mbClone(true)
                , mHisInfIdx(-1) {}

        ZSDHDRReqSetting(MINT8 exp, MINT8 order, Category category, MBOOL clone)
                : mExp(exp)
                , mOrder(order)
                , mCategory(category)
                , mbClone(clone)
                , mHisInfIdx(-1) {}
    };

    bool select_Zsl(SelectOutputParams& out,
                    SelectInputParams const& in,
                    ParsedZslRequestInfo const& parsedZslRequestInfo) const;

    void select_NonZsl(SelectOutputParams& out,
                       SelectInputParams const& in,
                       ParsedZslRequestInfo const& parsedZslRequestInfo) const;

    bool selectFromInflight(std::vector<FrameInfo>& selected,
                            int32_t const& requiredCnt,
                            std::vector<NSCam::ZslPolicy::InFlightInfo> const& inflightFrames) const;

    bool selectFromHistory(std::vector<FrameInfo>& vSelected,
                           int32_t const& requiredCnt,
                           std::map<HistoryFrameNoT, HistoryFrame> const& historyFrames,
                           int64_t const& zsdTimestamp) const;

    void generateOut(SelectOutputParams& out,
                     size_t const& zslFrameIdx,
                     ZSDHDRReqSetting const& reqSetting,
                     bool const& forceNonZSL = false) const;

    void setBufferIndex(IMetadata& meta,
                        FrameInfo const& frameInfo,
                        MINT8 const& exp) const;

    void extractBlobOrder(FrameInfo& frameInfo,
                          IMetadata const& meta) const;

    void dump(SelectOutputParams const& out) const;

    void decideReqSetting(ParsedZslRequestInfo const& parsedZslRequestInfo) const;

    void dumpReqSetting() const;

    void initRequestTable() const;

    ParsedZslRequestInfo parseZslRequestInfo(ZslRequestInfo const& zslRequestInfo) const;

  private:
    enum {
        Zsl_NonZsl_Mode = 0,
        Zsl_Mode = 1,
        NonZsl_Mode = 2
    };
    mutable MINT32 mMode;

    typedef std::tuple<MINT32, MUINT32> MappingKey;
    typedef std::vector<ZSDHDRReqSetting> MappingValue;
    mutable std::map<MappingKey, MappingValue> mRequestTable;

    mutable std::vector<ZSDHDRReqSetting> mvReqSetting;
    mutable MINT8 mZSLHistoryCnt;
    mutable MINT8 mZSLInflightCnt;

    mutable std::vector<FrameInfo> mvRecordHistoryFrame;
    mutable std::vector<FrameInfo> mvRecordInflightFrame;
    mutable MINT8 mCounter;

};
/******************************************************************************
 *
 ******************************************************************************/
};  // namespace Custom
};  // namespace NSCam

#endif  // _MTK_HARDWARE_MTKCAM_PIPELINE_MODEL_ZSL_CUSTOM_ZSDHDRSELECTOR_H_
