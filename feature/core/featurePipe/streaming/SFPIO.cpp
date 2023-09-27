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
 * MediaTek Inc. (C) 2010. All rights reserved.
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
#define PIPE_MODULE_TAG "SFPIO"
#define PIPE_CLASS_TAG "SFPIO"

#include <mtkcam3/feature/featurePipe/SFPIO.h>
#include <mtkcam3/feature/utils/p2/P2Util.h>
#include <featurePipe/core/include/PipeLog.h>

CAM_ULOG_DECLARE_MODULE_ID(MOD_FPIPE_STREAMING);


using NSCam::Feature::P2Util::P2IO;

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

IImageBuffer* get(const std::vector<IImageBuffer*> &imgs)
{
    return imgs.size() ? imgs[0] : NULL;
}

IImageBuffer* get(const std::vector<IImageBuffer*> &imgs, MUINT32 index)
{
    return (index < imgs.size()) ? imgs[index] : NULL;
}

/*******************************************
SFPOutput
*******************************************/

SFPOutput::SFPOutput(const std::vector<IImageBuffer*> &buffers, MUINT32 transform, OutTargetType targetType)
    : mBuffers(buffers)
    , mTransform(transform)
    , mTargetType(targetType)
    , mDMAConstrain(Feature::P2Util::DMAConstrain::DEFAULT)
{
    switch (targetType)
    {
        case OUT_TARGET_DISPLAY:    mType = P2IO::TYPE_DISPLAY;     break;
        case OUT_TARGET_RECORD:     mType = P2IO::TYPE_RECORD;      break;
        case OUT_TARGET_FD:         mType = P2IO::TYPE_FD;          break;
        case OUT_TARGET_APP_DEPTH:  mType = P2IO::TYPE_APP_DEPTH;   break;
        default:                    mType = P2IO::TYPE_VSS;         break;
    }
}

SFPOutput::SFPOutput()
    : mDMAConstrain(Feature::P2Util::DMAConstrain::DEFAULT)
{}

const char* SFPOutput::typeToChar(const OutTargetType &type)
{
    switch (type)
    {
        case OUT_TARGET_UNKNOWN:    return "unknown";
        case OUT_TARGET_DISPLAY:    return "disp";
        case OUT_TARGET_RECORD:     return "rec";
        case OUT_TARGET_FD:         return "fd";
        case OUT_TARGET_PHYSICAL:   return "phy";
        case OUT_TARGET_APP_DEPTH:  return "depth";
        default:                    return "invalid";
    }
}

MVOID SFPOutput::appendDumpInfo(LogString &str) const
{
    IImageBuffer *buffer = get(mBuffers);
    MSize size = (buffer != NULL) ? buffer->getImgSize() : MSize(0,0);
    eColorProfile prof = (buffer != NULL) ? buffer->getColorProfile() : eCOLORPROFILE_UNKNOWN;
    str.append("[buf(%p/%zu)(%dx%d),tran(%u),type(%d), tar(%s), prof(%d), crop(%f,%f,%fx%f), flag(0x%x)]",
                buffer, mBuffers.size(), size.w, size.h, mTransform, mType, typeToChar(mTargetType), (MUINT32)prof,
                mCropRect.p.x, mCropRect.p.y, mCropRect.s.w, mCropRect.s.h, mDMAConstrain);
}

MBOOL SFPOutput::isCropValid() const
{
    return (mCropRect.s.w > 0.0f && mCropRect.s.h > 0.0f && mCropDstSize.w > 0.0f && mCropDstSize.h > 0.0f);
}

P2IO SFPOutput::toP2IO() const
{
    return toP2IO(get(mBuffers));
}

std::vector<P2IO> SFPOutput::toP2IOVector() const
{
    std::vector<P2IO> vector;
    vector.reserve(mBuffers.size());
    for( MUINT32 i = 0, n = mBuffers.size(); i < n; ++i )
    {
        vector.emplace_back(toP2IO(mBuffers[i]));
    }
    return vector;
}

P2IO SFPOutput::toP2IO(IImageBuffer *buffer) const
{
    return P2IO(buffer, mTransform, mType,
                mCropRect, mCropDstSize, mDMAConstrain);
}

/*******************************************
SFPSensorInput
*******************************************/
MVOID SFPSensorInput::appendDumpInfo(LogString &str, MUINT32 sID) const
{
    str.append("[sID(%d)--IMG(%p/%zu),RRZ(%p/%zu),LCS(%p/%zu),LCSH(%p/%zu),pRSS(%p/%zu),cRSS(%p/%zu),RSSR2(%p/%zu),FYuv(%p/%zu),RYuv1(%p/%zu),RYuv2(%p/%zu),mAIYuv(%p/%zu),HalI(%p),AppI(%p),AppDI(%p),AppOver(%p)]",
                sID,
                get(mIMGO), mIMGO.size(),
                get(mRRZO), mRRZO.size(),
                get(mLCSO), mLCSO.size(),
                get(mLCSHO), mLCSHO.size(),
                get(mPrvRSSO), mPrvRSSO.size(),
                get(mCurRSSO), mCurRSSO.size(),
                get(mRSSOR2), mRSSOR2.size(),
                get(mFullYuv), mFullYuv.size(),
                get(mRrzYuv1), mRrzYuv1.size(),
                get(mRrzYuv2), mRrzYuv2.size(),
                get(mAIYuv), mAIYuv.size(),
                mHalIn, mAppIn,mAppDynamicIn, mAppInOverride);
}

NSCam::IMetadata* SFPSensorInput::getValidAppIn() const
{
    return mAppIn ? mAppIn : mAppInOverride;
}

/*******************************************
SFPSensorTuning
*******************************************/

MBOOL SFPSensorTuning::isRRZOin() const
{
    return (mFlag & FLAG_RRZO_IN);
}

MBOOL SFPSensorTuning::isIMGOin() const
{
    return (mFlag & FLAG_IMGO_IN);
}
MBOOL SFPSensorTuning::isLCSOin() const
{
    return (mFlag & FLAG_LCSO_IN);
}
MBOOL SFPSensorTuning::isLCSHOin() const
{
    return (mFlag & FLAG_LCSHO_IN);
}
MBOOL SFPSensorTuning::isDisable3DNR() const
{
    return (mFlag & FLAG_FORCE_DISABLE_3DNR);
}

MBOOL SFPSensorTuning::isAppPhyMetaIn() const
{
    return (mFlag & FLAG_APP_PHY_META_IN);
}

MVOID SFPSensorTuning::addFlag(Flag flag)
{
    mFlag |= flag;
}

MBOOL SFPSensorTuning::isValid() const
{
    return mFlag != 0;
}

MVOID SFPSensorTuning::appendDumpInfo(LogString &str) const
{
    str.append("[flag(%d)]", mFlag);
}

/*******************************************
SFPIOMap
*******************************************/

const char* SFPIOMap::pathToChar(const PathType &type)
{
    switch (type)
    {
        case PATH_GENERAL:
            return "GEN";
        case PATH_PHYSICAL:
            return "PHY";
        case PATH_LARGE:
            return "LARGE";
        case PATH_UNKNOWN:
        default:
            return "invalid";
    }
}

MVOID SFPIOMap::addInputTuning(MUINT32 sensorID, const SFPSensorTuning& input)
{
    mInputMap[sensorID] = input;
}

MBOOL SFPIOMap::hasTuning(MUINT32 sensorID) const
{
    return mInputMap.count(sensorID) > 0;
}

const SFPSensorTuning& SFPIOMap::getTuning(MUINT32 sensorID) const
{
    if(mInputMap.count(sensorID) > 0)
        return mInputMap.at(sensorID);
    else
        return mDummy;
}

MVOID SFPIOMap::addOutput(const SFPOutput& out)
{
    mOutList.push_back(out);
}

MVOID SFPIOMap::getAllOutput(std::vector<SFPOutput> &outList) const
{
    outList = mOutList;
}

MVOID SFPIOMap::getAllOutput(std::vector<P2IO> &outList) const
{
    size_t size = mOutList.size();
    outList.clear();
    outList.reserve(size);
    for( unsigned i = 0; i < size; ++i )
    {
        outList.emplace_back(mOutList[i].toP2IO());
    }

}

MBOOL SFPIOMap::isValid() const
{
    return (mPathType != PATH_UNKNOWN) && (! mOutList.empty());
}
MBOOL SFPIOMap::isGenPath() const
{
    return (mPathType == PATH_GENERAL);
}

const char* SFPIOMap::pathName() const
{
    return pathToChar(mPathType);
}

MVOID SFPIOMap::appendDumpInfo(LogString &str) const
{
    str.append("{path(%s),halO(%p),appO(%p),Outs--", pathToChar(mPathType), mHalOut, mAppOut);
    for(auto& out : mOutList)
        out.appendDumpInfo(str);

    for(auto& in : mInputMap)
    {
        str.append("Tuning--id(%u)--",in.first);
        in.second.appendDumpInfo(str);
    }
    str.append("}");
}

MUINT32 SFPIOMap::getFirstSensorID()
{
    if(mFirstID != INVALID_SENSOR_ID)
        return mFirstID;
    for(auto& it : mInputMap)
    {
        mFirstID = it.first;
    }
    return mFirstID;
}

MVOID SFPIOMap::getAllSensorIDs(std::vector<MUINT32>& ids) const
{
    for(auto& it : mInputMap)
    {
        ids.push_back(it.first);
    }
}

MBOOL SFPIOMap::isSameTuning(const SFPIOMap &map1, const SFPIOMap &map2, MUINT32 sensorID)
{
    if(!map1.isValid() || !map2.isValid())
        return MFALSE;
    const SFPSensorTuning& tun1 = map1.getTuning(sensorID);
    const SFPSensorTuning& tun2 = map2.getTuning(sensorID);
    if(!tun1.isValid() || !tun2.isValid())
        return MFALSE;
    return (tun1.mFlag == tun2.mFlag);
}

/*******************************************
SFPIOManager
*******************************************/
MBOOL SFPIOManager::addInput(MUINT32 sensorID, SFPSensorInput &input)
{
    mSensorInputs[sensorID] = input;
    return MTRUE;
}

MBOOL SFPIOManager::addGeneral(SFPIOMap &sfpio)
{
    mGenerals.push_back(sfpio);

    MBOOL isOneGeneral = (mGenerals.size() <= 1);
    if(!isOneGeneral)
        MY_LOGF("Currently not allow more than 1 general");
    return isOneGeneral;
}

MBOOL SFPIOManager::addPhysical(MUINT32 sensorID, SFPIOMap &sfpio)
{
    if(mPhysicals.count(sensorID) > 0 || sensorID == INVALID_SENSOR_ID)
        return MFALSE;
    mPhysicals[sensorID] = sfpio;
    return MTRUE;
}

MBOOL SFPIOManager::addLarge(MUINT32 sensorID, SFPIOMap &sfpio)
{
    if(mLarges.count(sensorID) > 0 || sensorID == INVALID_SENSOR_ID)
        return MFALSE;
    mLarges[sensorID] = sfpio;
    return MTRUE;
}

const SFPSensorInput& SFPIOManager::getInput(MUINT32 sensorID)
{
    if(mSensorInputs.count(sensorID) > 0)
        return mSensorInputs.at(sensorID);
    else
        return mDummyInput;
}

const std::vector<SFPIOMap>& SFPIOManager::getGeneralIOs() const
{
    return mGenerals;
}

MUINT32 SFPIOManager::countAll() const
{
    return mGenerals.size() + mPhysicals.size() + mLarges.size();
}

MUINT32 SFPIOManager::countNonLarge() const
{
    return mGenerals.size() + mPhysicals.size();
}

MUINT32 SFPIOManager::countLarge() const
{
    return mLarges.size();
}

MUINT32 SFPIOManager::countPhysical() const
{
    return mPhysicals.size();
}

MUINT32 SFPIOManager::countGeneral() const
{
    return mGenerals.size();
}

const SFPIOMap& SFPIOManager::getFirstGeneralIO() const
{
    if(mGenerals.size() > 0)
        return mGenerals.front();
    else
        return mDummy;
}

MVOID SFPIOManager::appendDumpInfo(LogString &str) const
{
    for(auto& it : mSensorInputs)
        it.second.appendDumpInfo(str, it.first);
    for(auto&& io : mGenerals)
        io.appendDumpInfo(str);
    for(auto&& it : mPhysicals)
        it.second.appendDumpInfo(str);
    for(auto&& it : mLarges)
        it.second.appendDumpInfo(str);
}


} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam
