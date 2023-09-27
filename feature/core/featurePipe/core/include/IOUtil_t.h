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
 * MediaTek Inc. (C) 2017. All rights reserved.
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

#ifndef _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_T_H_
#define _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_T_H_

#include <set>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "MtkHeader.h"
#include "BufferPool.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

typedef MUINT32 SENSOR_ID;

enum StreamType {
  STREAMTYPE_PREVIEW,
  STREAMTYPE_PREVIEW_CALLBACK,
  STREAMTYPE_RECORD,
  STREAMTYPE_FD,
  STREAMTYPE_PHYSICAL,
  STREAMTYPE_TINY,
  STREAMTYPE_ASYNC,

  STREAMTYPE_ENUM_COUNT,
};

class StreamMask {
public:
  MVOID enableAll() { mMask = ~0; }
  MVOID disableAll() { mMask = 0; }
  MVOID enable(StreamType stream) { mMask = (mMask | (1 << stream)); }
  MVOID disable(StreamType stream) { mMask = (mMask & ~(1 << stream)); }
  MBOOL has(StreamType stream) const { return (mMask & (1 << stream)); }
  MBOOL hasAny() const { return mMask > 0; }
private:
  MUINT32 mMask = 0;
};

enum OutputType {
  OUTPUT_INVALID,

  OUTPUT_STREAM_PREVIEW,
  OUTPUT_STREAM_PREVIEW_CALLBACK,
  OUTPUT_STREAM_RECORD,
  OUTPUT_STREAM_FD,
  OUTPUT_STREAM_PHYSICAL,
  OUTPUT_STREAM_TINY,
  OUTPUT_STREAM_ASYNC,

  OUTPUT_FULL,
  OUTPUT_NEXT_FULL,

  OUTPUT_ENUM_COUNT,
};


template <typename ATTR_T>
class NextBuf {
public:
  android::sp<IIBuffer> mBuffer;
  ATTR_T mAttr;
};

template <typename ATTR_T>
class NextPool {
public:
  android::sp<IBufferPool> mPool;
  ATTR_T mAttr;
public:
  NextPool(const android::sp<IBufferPool> &pool = NULL, const ATTR_T &attr = ATTR_T());
  MBOOL isValid() const;
};

enum IOPolicyType {
    IOPOLICY_BYPASS,
    IOPOLICY_INOUT,
    IOPOLICY_LOOPBACK,
    IOPOLICY_INOUT_EXCLUSIVE,
    IOPOLICY_INOUT_NEXT,
    IOPOLICY_ENUM_COUNT,
};

template <typename User_T>
class IOPolicy {

public:
  typedef typename User_T::IO_PathID PathID_T;
  typedef typename User_T::IO_NextID NextID_T;
  typedef typename User_T::IO_NextAttr ATTR_T;
  typedef typename std::map<NextID_T, NextPool<ATTR_T>> NPoolMap;
  /*
  * If set true, this node has no color conversion/resize/crop function.
  * Previous node must use next node's policy to decide its policy is NextFull or Full.
  */
  MVOID setNoMDP(MBOOL noMDP);
  MVOID setLoopBack(MBOOL loopback, MUINT32 sensorID);
  MVOID setRun(const StreamMask &runPath, MUINT32 sensorID);
  MVOID setUseAdvSetting(MBOOL useAdv);
  MVOID enableAll(MUINT32 sensorID);
  MVOID add(MUINT32 sensorID, const NextID_T &id, const ATTR_T &attr, const android::sp<IBufferPool> &pool);
  MBOOL isRun(StreamType stream, MUINT32 sensorID) const;
  MBOOL isNoMDP() const;
  MBOOL isLoopBack(MUINT32 sensorID) const;
  std::string toStr() const;
  /* The map may only have some pathID. If target pathID doesn't exist in map,
   * It will return default path ID.
   */
  PathID_T getAvailablePath(MUINT32 sensorID, PathID_T pathID) const;
  const NPoolMap& getInputMap(MUINT32 sensorID, PathID_T pathID) const;
  MUINT32 getInputMapCount(MUINT32 sensorID) const;
  IOPolicyType calPolicyType(PathID_T path, MUINT32 sensorID) const;
private:
  MVOID add(MUINT32 sensorID, PathID_T path, const NextID_T &id, const NextPool<ATTR_T> &pool);
  typedef std::map<PathID_T, NPoolMap> NPoolPathMap;
  class SensorPolicy {
  public:
    StreamMask mRunPath;
    MBOOL mLoopBack = MFALSE; // If set true, this node must output FULL even if it has no next node.
    NPoolPathMap mPathPoolMap;

  };
  MBOOL mUseAdvSetting = MFALSE;
  MBOOL mNoMDP = MFALSE;
  std::map<SENSOR_ID, SensorPolicy> mSensorPolicys;
  NPoolMap mDummyPoolMap;
};

template <typename Node_T, typename ReqInfo_T, typename User_T>
class IORequest;

template <typename Node_T, typename ReqInfo_T, typename User_T>
class IOControl
{
public:
  friend class IORequest<Node_T, ReqInfo_T, User_T>;
  using IORequest = IORequest<Node_T, ReqInfo_T, User_T>;
  using IOPolicy = IOPolicy<User_T>;
  using NPoolMap = typename IOPolicy::NPoolMap;
  typedef typename User_T::IO_PathID PathID_T; // for image input path
  typedef typename User_T::IO_NextID NextID_T; // for image
  typedef typename User_T::IO_NextAttr ATTR_T;
  typedef typename User_T::IO_TargetID TargetID_T; // for Node output buffers to next node
  using NextPool = NextPool<ATTR_T>;
  using NextBuf = NextBuf<ATTR_T>;
  typedef typename std::list<Node_T*> NodeList;
  typedef std::set<StreamType> StreamSet;

  typedef typename std::map<NextID_T, NextBuf> NextBufMap;
  typedef typename std::map<TargetID_T, NextBufMap> NextTargetMap;

  class NextImgReq {
  public:
    class User
    {
    public:
        TargetID_T mTargetID = User_T::DefaultTargetID_ID;
        NextID_T mID;
        User(TargetID_T target = User_T::DefaultTargetID_ID, const NextID_T &id = NextID_T())
        : mTargetID(target)
        , mID(id)
        {}
    };
    NextID_T mID;
    NextBuf mImg;
    std::list<User> mUsers;
  };

  typedef std::list<NextImgReq> NextImgReqList;
  class NextCollection
  {
  public:
    NextImgReqList mList;
    NextTargetMap mMap;
  };

  IOControl();
  ~IOControl();

  MVOID setRoot(Node_T *root);
  MVOID addStream(const StreamType &stream, const NodeList &nodes);
  MBOOL prepareMap(const std::map<SENSOR_ID, StreamSet> &streams, const ReqInfo_T &reqInfo, std::unordered_map<SENSOR_ID, IORequest> &reqMap);
  MVOID dump(const IORequest &req) const;
  MVOID dump(const std::unordered_map<SENSOR_ID, IORequest> &reqMap) const;

private:
  typedef typename std::set<Node_T*> NodeSet;
  struct PolicyInfo
  {
      PolicyInfo() {}
      PolicyInfo(Node_T *node, MBOOL run, MUINT32 pathCnt, IOPolicyType policyType)
        : mNode(node), mRun(run), mAvailablePathCnt(pathCnt), mPolicyType(policyType) {}

      Node_T *mNode = NULL;
      MBOOL mRun = MFALSE;
      MUINT32 mAvailablePathCnt = 0;
      IOPolicyType mPolicyType = IOPOLICY_BYPASS;
  };

  class NextInfo
  {
  public:
    NextInfo() {}
    NextInfo(const Node_T *node, MUINT32 pathCnt);
    void addType(const OutputType &type);
    bool needType(const OutputType &type) const;
    void addNext(Node_T *next, PathID_T path);
    bool hasNext(Node_T *next, PathID_T path) const;
    void setTargetNode(TargetID_T target, Node_T *next);
    bool isTargetOccupied(TargetID_T target, Node_T *next) const;
    void addInputPath(PathID_T pathInPolicy, TargetID_T targetPath);
    bool hasInput(PathID_T pathInPolicy) const;
    TargetID_T getInput(PathID_T pathInPolicy) const;

    void print(const char *path) const;
  private:
    struct NodePath
    {
        Node_T *mNode = NULL;
        PathID_T mPath = User_T::DefaultPathID_ID;
        NodePath(Node_T *node, PathID_T path)
        : mNode(node)
        , mPath(path)
        {}
        bool operator < (const NodePath &o)  const {
            return (mNode < o.mNode) || (mPath < o.mPath);
        }
    };
    const Node_T *mNode = NULL;
    MUINT32 mInPathCnt = 0;
    std::set<NodePath> mNexts;
    std::map<TargetID_T, Node_T*> mTargets;
    std::set<OutputType> mOutput;
    std::map<PathID_T, TargetID_T> mInput;
  };
  typedef std::map<StreamType, NodeList> PathMap;
  typedef std::map<const Node_T*, NextCollection> BufferTable;
  typedef std::map<const Node_T*, NextInfo> NextTable;
  typedef std::list<PolicyInfo> PolicyInfoList;
  typedef std::map<const Node_T*, IOPolicy> PolicyMap; // store all nodes return policy data

  MVOID dumpInfo(const NextTable &nextTable, const StreamSet &streamSet) const;
  MVOID dumpInfo(const BufferTable &bufferTable) const;

  MBOOL prepareStreamMap(MUINT32 sensorID, const StreamType &stream, const PolicyMap &policyMap, IORequest &req);
  MBOOL prepareAsyncNext(MUINT32 sensorID, const PolicyMap &policyMap, const PolicyInfoList &policys, PathID_T pathID, Node_T* &asyncNext, NextBuf &asyncBuf);
  PolicyInfoList queryPathPolicy(MUINT32 sensorID, const StreamType &stream, const PolicyMap &policyMap, PathID_T pathID) const;
  MBOOL preparePolicyInfos(const ReqInfo_T &reqInfo, PolicyMap &policyMap) const;

  MBOOL appendNextBufMap(const Node_T *node, const NPoolMap &poolMap, NextBufMap &out) const;
  MVOID mergeBufMap(BufferTable &bufferTable) const;
  NextImgReqList toList(const NextBufMap &map, TargetID_T targetID) const;
  MVOID tryMerge(TargetID_T targetID, NextImgReqList &list, size_t searchRange, const NextID_T &id, NextBuf &img) const;

  NextPool getFirstNextPool(const PolicyMap &policyMap, MUINT32 sensorID, Node_T *node, PathID_T pathID) const;

  MBOOL backwardCalc(MUINT32 sensorID,
                    const StreamType &stream,
                    const PolicyMap &policyMap,
                    const PolicyInfoList &policyList,
                    PathID_T pathID,
                    IORequest &req) const;

  Node_T                     *mRoot = NULL;
  PathMap                     mStreamPathMap;
  NodeSet                     mAllNodeSet;
};

template <typename Node_T, typename ReqInfo_T, typename User_T>
class IORequest
{
public:
  friend class IOControl<Node_T, ReqInfo_T, User_T>;
  using IOControl = IOControl<Node_T, ReqInfo_T, User_T>;
  using NextBufMap = typename IOControl::NextBufMap;
  using NextTargetMap = typename IOControl::NextTargetMap;
  using NextCollection = typename IOControl::NextCollection;
  using ATTR_T = typename IOControl::ATTR_T;
  using NextBuf = typename IOControl::NextBuf;

  MBOOL isFullBufHold() const;
  MBOOL needPreview(const Node_T *node) const;
  MBOOL needPreviewCallback(const Node_T *node) const;
  MBOOL needRecord(const Node_T *node) const;
  MBOOL needFD(const Node_T *node) const;
  MBOOL needPhysicalOut(const Node_T *node) const;
  MBOOL needFull(const Node_T *node) const;
  MBOOL needNextFull(const Node_T *node) const;
  MBOOL needAsync(const Node_T *node) const;
  NextCollection getNextFullImg(const Node_T *node);
  NextBuf getAsyncImg(const Node_T *node);
  MVOID clearAsyncImg(const Node_T *node);

  MBOOL needOutputType(const Node_T *node, const OutputType &type) const;

private:
  using NextTable = typename IOControl::NextTable;
  using BufferTable = typename IOControl::BufferTable;
  using StreamSet = typename IOControl::StreamSet;
  NextTable mNextTable;
  BufferTable mBufferTable;
  StreamSet mStreamSet;

  Node_T *mAsyncNext = NULL;
  NextBuf mAsyncBuffer;
};

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#endif // _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_T_H_
