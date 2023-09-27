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

#ifndef  _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_ZSDHDRPLUGIN_H_
#define  _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_ZSDHDRPLUGIN_H_

#include <stdlib.h>

#include <vector>
#include <map>

#include <mtkcam3/3rdparty/plugin/PipelinePlugin.h>
#include <mtkcam3/3rdparty/plugin/PipelinePluginType.h>
#include <mtkcam3/def/zslPolicy/types.h>

using NSCam::ZslPolicy::ISelector;

namespace NSCam {
namespace NSPipelinePlugin {
/******************************************************************************
 *
 ******************************************************************************/
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
            , mbClone(false)
            , mHisInfIdx(-1) {}

    ZSDHDRReqSetting(MINT8 exp, MINT8 order, Category category, MBOOL clone)
            : mExp(exp)
            , mOrder(order)
            , mCategory(category)
            , mbClone(clone)
            , mHisInfIdx(-1) {}
};

class ZSDHDRSelector : public ISelector
{
  public:
    using NSCam::ZslPolicy::ISelector::SelectOutputParams;
    using NSCam::ZslPolicy::ISelector::SelectInputParams;

    ZSDHDRSelector(std::vector<ZSDHDRReqSetting>& vReqSetting);

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

    void select_NonZsl(SelectOutputParams& out,
                       SelectInputParams const& in) const;

    bool selectFromHistory(std::vector<FrameInfo>& selected,
                           int32_t const& requiredCnt,
                           std::map<HistoryFrameNoT, HistoryFrame> const& historyFrames,
                           int64_t const& zsdTimestamp) const;

    bool selectFromInFlight(std::vector<FrameInfo>& selected,
                            int32_t const& requiredCnt,
                            std::vector<ZslPolicy::InFlightInfo> const& inflightFrames) const;

    void extractBlobOrder(FrameInfo& frameInfo,
                          IMetadata const& meta) const;

    void setBufferIndex(IMetadata& meta,
                        FrameInfo const& frameInfo,
                        MINT8 const& exp) const;

    void generateOut(SelectOutputParams& out,
                     size_t const& zslFrameIdx,
                     ZSDHDRReqSetting const& reqSetting,
                     bool const& forceNonZSL = false) const;

    void dump(const SelectOutputParams& out) const;

    void dumpReqSetting() const;

    void init() const;

  private:
    mutable std::vector<ZSDHDRReqSetting> mvReqSetting;
    mutable MINT8 mZSLHistoryCnt;
    mutable MINT8 mZSLInflightCnt;

    mutable std::vector<FrameInfo> mvRecordHistoryFrame;
    mutable std::vector<FrameInfo> mvRecordInflightFrame;
};

/******************************************************************************
 *
 ******************************************************************************/
class ZSDHDRPluginProviderImp : public MultiFramePlugin::IProvider
{
    typedef MultiFramePlugin::Property Property;
    typedef MultiFramePlugin::Selection Selection;
    typedef MultiFramePlugin::Request::Ptr RequestPtr;
    typedef MultiFramePlugin::RequestCallback::Ptr RequestCallbackPtr;

  public:

    ZSDHDRPluginProviderImp();

    virtual ~ZSDHDRPluginProviderImp();

    virtual const Property& property() override;

    virtual MERROR negotiate(Selection& sel) override;

    virtual void init() override;

    virtual MERROR process(RequestPtr pRequest,
                           RequestCallbackPtr pCallback = nullptr) override;

    virtual void abort(std::vector<RequestPtr>& pRequests) override;

    virtual void uninit() override;

  private:
    void initRequestTable();

    void dumpBuffer(const IMetadata* pHalMeta,
                    const MINT32 sensorId,
                    IImageBuffer* pIImgBuffer,
                    const char* pUserString = "");

    void writeFeatureDecisionLog(int32_t sensorId,
                                 IMetadata* halInMeta,
                                 const char* log);

  private:
    enum {
        Zsl_NonZsl_Mode = 0,
        Zsl_Mode = 1,
        NonZsl_Mode = 2
    };

    enum {
        YuvDomainHDR = 0,
        RawDomainHDR = 1
    };

    MINT32                          mEnable;
    MINT32                          mMode;

    MINT32                          mAlgoType;

    MINT32                          mEnableLog;
    MINT32                          mDumpBuffer;

    typedef std::tuple<MINT32, MUINT32> MappingKey;
    typedef std::vector<ZSDHDRReqSetting> MappingValue;
    std::map<MappingKey, MappingValue> mRequestTable;

    MUINT8                          mZsdBufferMaxNum;

    MUINT32                         mHDRHalMode;
    std::shared_ptr<ISelector>      mSelector;

    std::vector<RequestPtr>         mvpRequest;
    RequestCallbackPtr              mpCallback;
};

/******************************************************************************
 *
 ******************************************************************************/
};  // namespace NSPipelinePlugin
};  // namespace NSCam

#endif  // _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PLUGIN_PIPELINEPLUGIN_ZSDHDRPLUGIN_H_
