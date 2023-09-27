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

#include "P2GDefine.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

const char* toName(Scene scene)
{
  switch(scene)
  {
  case Scene::GM:  return "GM";
  case Scene::GS:  return "GS";
  case Scene::PM:  return "PM";
  case Scene::PS:  return "PS";
  case Scene::LM:  return "LM";
  case Scene::LS:  return "LS";
  default:                  return "unknown";
  }
}

const char* toName(Feature::FID fid)
{
  switch(fid)
  {
  case Feature::NONE:          return "none";
  case Feature::DUAL:          return "dual";
  case Feature::SMVR_MAIN:     return "smvr_main";
  case Feature::SMVR_SUB:      return "smvr_sub";
  case Feature::NR3D:          return "nr3d";
  case Feature::DSDN30_INIT:   return "dsdn30_init";
  case Feature::DSDN30:        return "dsdn30";
  case Feature::DSDN25:        return "dsdn25";
  case Feature::DSDN20:        return "dsdn20";
  case Feature::DCE:           return "dce";
  case Feature::DRE_TUN:       return "dre_tun";
  case Feature::TIMGO:         return "timgo";
  case Feature::EARLY_DISP:    return "early_disp";
  default:                     return "unknown";
  }
}

LoopScene toLoopScene(Scene scene)
{
    switch(scene)
    {
    case Scene::GM:
    case Scene::GS:
        return LoopScene::GEN;
    case Scene::PM:
    case Scene::PS:
        return LoopScene::PHY;
    case Scene::LM:
    case Scene::LS:
        return LoopScene::LAR;
    default:
        return LoopScene::GEN;
    }
}

Feature::Feature()
{
}

Feature::Feature(unsigned mask)
  : mMask(mask)
{
}

void Feature::add(unsigned mask)
{
  mMask |= mask;
}

void Feature::operator+=(unsigned mask)
{
  add(mask);
}

void Feature::operator-=(unsigned mask)
{
  mMask &= ~mask;
}

Feature Feature::operator+(Feature rhs) const
{
  rhs.add(this->mMask);
  return rhs;
}

bool Feature::has(unsigned mask) const
{
  return mMask & mask;
}

bool Feature::hasDUAL() const
{
  return has(DUAL);
}

bool Feature::hasSMVR_MAIN() const
{
  return has(SMVR_MAIN);
}

bool Feature::hasSMVR_SUB() const
{
  return has(SMVR_MAIN);
}

bool Feature::hasNR3D() const
{
  return has(NR3D);
}

bool Feature::hasDCE() const
{
  return has(DCE);
}

bool Feature::hasDRE_TUN() const
{
  return has(DRE_TUN);
}

bool Feature::hasTIMGO() const
{
  return has(TIMGO);
}

bool Feature::hasDSDN20() const
{
  return has(DSDN20);
}

bool Feature::hasDSDN25() const
{
  return has(DSDN25);
}

bool Feature::hasDSDN30() const
{
  return has(DSDN30) || has(DSDN30_INIT);
}

bool IO::has(Feature::FID fid) const
{
    return mFeature.has(fid);
}

IO::LoopData IO::makeLoopData()
{
    return std::make_shared<IO_LoopData>();
}

IO::BatchData IO::makeBatchData()
{
    return std::make_shared<IO_BatchData>();
}

bool isMaster(Scene scene)
{
  switch( scene )
  {
  case Scene::GM:  return true;
  case Scene::PM:  return true;
  case Scene::LM:  return true;
  case Scene::GS:  return false;
  case Scene::PS:  return false;
  case Scene::LS:  return false;
  default:  return true;
  };
}

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe
