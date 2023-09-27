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

#ifndef _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_PATH_ENGINE_H_
#define _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_PATH_ENGINE_H_

#include "P2GDefine.h"

#include <vector>

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {
namespace P2G {

class PathEngine
{
public:
  PathEngine();
  ~PathEngine();
  bool init(const DataPool &dataPool, const Feature &featureSet, bool is4K, bool is60fps,bool supportIMG3O);
  bool uninit();
  Path run(const std::vector<IO> &ioList);

private:
  bool initCheck(const DataPool &pool, const Feature &feature) const;

  void estimatePathCount(const std::vector<IO> &ioList, size_t &cPMss, size_t &cMss, size_t &cP2SW, size_t &cP2HW);
  void generatePath(const IO &io, Path &path);
  void generatePreSubRun(const IO &io, Path &path, SubIO &lastSub);
  void generateMainRun(const IO &io, Path &path, const SubIO &lastSub);
  void generatePostSubRun(const IO &io, Path &path);

  MSS prepareMSS(const IO &io);
  MSS preparePMSS(const IO &io, const MSS &mss);
  bool prepareP2SW(const Feature &f, const IO &io, const SubIO &subIO, unsigned dsdnIndex, P2SW &p2sw);
  bool prepareP2HW(const Feature &f, const IO &io, const SubIO &subIO, unsigned dsdnIndex, const P2SW &p2sw, P2HW &p2hw, PMDP &pmdp);
  void fillFull(P2HW &p2hw, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &full);
  bool prepareP2GOut(const IO &io, P2HW &p2hw, PMDP &pmdp);
  void prepareBasicLoopOut(const IO &io);
  void updateLoopOut(const IO &io, unsigned dsdnIndex, const P2SW &p2sw, const P2HW &p2hw);
  void updateBatchOut(const IO &io, unsigned dsdnIndex, const P2SW &p2sw);

  bool allowAppOut(const IO &io) const;
  bool needLoop() const;
  bool needBatchIn(const IO &io) const;
  bool needBatchOut(const IO &io) const;
  bool validate(const IO &io) const;
  bool validate(bool cond, const char *msg) const;

private:
  static void fill(P2HW &p2hw, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &io);
  static void fill(P2HW &p2hw, PMDP &pmdp, bool &wdmaUsed, bool &wrotUsed, const ILazy<GImg> &io, bool supMultiRotate);
  static bool tryFillRotate(P2HW &p2hw, bool &wrotUsed, const std::vector<ILazy<GImg>> &list);
  static void fillAll(P2HW &p2hw, PMDP &pmdp, bool &wdmaUsed, bool &wrotUsed, const std::vector<ILazy<GImg>> &list, bool skipFirstRotate, bool supMultiRotate);

private:
  enum class State { NONE, READY, FAIL };

  State mState = State::NONE;
  DataPool mPool;
  Feature mFeatureSet;
  bool mSupMultiRotate = false;
  bool mUseIMG3O = true;
  bool mIs4K = false;
  bool mIs60FPS = false;
};

} // namespace P2G
} // namespace NSCam
} // namespace NSCamFeature
} // namespace NSFeaturePipe

#endif // _MTK_CAMERA_STREAMING_FEATURE_PIPE_P2G_CORE_PATH_ENGINE_H_
