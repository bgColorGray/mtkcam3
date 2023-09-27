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

#ifndef _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_H_
#define _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_H_

#include "IOUtil_t.h"

#include "PipeLogHeaderBegin.h"
#include "StringUtil.h"
#include "DebugControl.h"
#define PIPE_TRACE TRACE_IO_UTIL
#define PIPE_CLASS_TAG "IOUtil"
#define PIPE_ULOG_TAG NSCam::Utils::ULog::MOD_FPIPE_COMMON
#include "PipeLog.h"

namespace NSCam {
namespace NSCamFeature {
namespace NSFeaturePipe {

template<typename T>
static inline const char* getName(const T *node)
{
  return node ? node->getName() : "NULL";
}

static inline OutputType streamToType(const StreamType &stream)
{
  switch(stream)
  {
    case STREAMTYPE_PREVIEW:          return OUTPUT_STREAM_PREVIEW;
    case STREAMTYPE_PREVIEW_CALLBACK: return OUTPUT_STREAM_PREVIEW_CALLBACK;
    case STREAMTYPE_RECORD:           return OUTPUT_STREAM_RECORD;
    case STREAMTYPE_FD:               return OUTPUT_STREAM_FD;
    case STREAMTYPE_PHYSICAL:         return OUTPUT_STREAM_PHYSICAL;
    case STREAMTYPE_TINY:             return OUTPUT_STREAM_TINY;
    case STREAMTYPE_ASYNC:            return OUTPUT_STREAM_ASYNC;
    default:                          return OUTPUT_INVALID;
  }
}

static inline const char* toName(const StreamType &stream)
{
  switch(stream)
  {
    case STREAMTYPE_PREVIEW:          return "preview";
    case STREAMTYPE_PREVIEW_CALLBACK: return "preview_callback";
    case STREAMTYPE_RECORD:           return "record";
    case STREAMTYPE_FD:               return "fd";
    case STREAMTYPE_PHYSICAL:         return "phy";
    case STREAMTYPE_TINY:             return "tiny";
    case STREAMTYPE_ASYNC:            return "async";
    default:                          return "unknown";
  }
}

static inline const char* toName(const OutputType &type)
{
  switch(type)
  {
    case OUTPUT_INVALID:                  return "invalid";
    case OUTPUT_STREAM_PREVIEW:           return "preview";
    case OUTPUT_STREAM_PREVIEW_CALLBACK:  return "preview_callback";
    case OUTPUT_STREAM_RECORD:            return "record";
    case OUTPUT_STREAM_FD:                return "fd";
    case OUTPUT_STREAM_PHYSICAL:          return "phy_out";
    case OUTPUT_STREAM_TINY:              return "tiny";
    case OUTPUT_STREAM_ASYNC:             return "async";
    case OUTPUT_FULL:                     return "full";
    case OUTPUT_NEXT_FULL:                return "next_full";
    default:                              return "unknown";
  }
}

static inline const char* toName(const IOPolicyType &policy)
{
  switch(policy)
  {
    case IOPOLICY_BYPASS:           return "bypass";
    case IOPOLICY_INOUT:            return "inout";
    case IOPOLICY_LOOPBACK:         return "loopback";
    case IOPOLICY_INOUT_EXCLUSIVE:  return "inout_e";
    case IOPOLICY_INOUT_NEXT:       return "inout_n";
    default:                        return "unknown";
  }
}

template <typename ATTR_T>
NextPool<ATTR_T>::NextPool(const android::sp<IBufferPool> &pool, const ATTR_T &attr)
  : mPool(pool)
  , mAttr(attr)
{
}

template <typename ATTR_T>
MBOOL NextPool<ATTR_T>::isValid() const
{
    return (mAttr.needBuffer() && mPool != NULL)
            || (!mAttr.needBuffer() && mPool == NULL);
}

template <typename User_T>
MVOID IOPolicy<User_T>::setNoMDP(MBOOL noMDP)
{
    mNoMDP = noMDP;
}

template <typename User_T>
MVOID IOPolicy<User_T>::setLoopBack(MBOOL loopback, MUINT32 sensorID)
{
    mSensorPolicys[sensorID].mLoopBack = loopback;
}

template <typename User_T>
MVOID IOPolicy<User_T>::setRun(const StreamMask &runPath, MUINT32 sensorID)
{
    mSensorPolicys[sensorID].mRunPath = runPath;
}

template <typename User_T>
MVOID IOPolicy<User_T>::setUseAdvSetting(MBOOL useAdv)
{
    mUseAdvSetting = useAdv;
}

template <typename User_T>
MVOID IOPolicy<User_T>::enableAll(MUINT32 sensorID)
{
    mSensorPolicys[sensorID].mRunPath.enableAll();
}

template <typename User_T>
MVOID IOPolicy<User_T>::add(MUINT32 sensorID, const NextID_T &id, const ATTR_T &attr, const android::sp<IBufferPool> &pool)
{
    add(sensorID, id.mPath, id, NextPool<ATTR_T>(pool, attr));
}

template <typename User_T>
MVOID IOPolicy<User_T>::add(MUINT32 sensorID, PathID_T path, const NextID_T &id, const NextPool<ATTR_T> &pool)
{
    mSensorPolicys[sensorID].mPathPoolMap[path][id] = pool;
}

template <typename User_T>
MBOOL IOPolicy<User_T>::isRun(StreamType stream, MUINT32 sensorID) const
{
  return mSensorPolicys.count(sensorID) ? mSensorPolicys.at(sensorID).mRunPath.has(stream) : MFALSE;
}

template <typename User_T>
MBOOL IOPolicy<User_T>::isNoMDP() const
{
  return mNoMDP;
}

template <typename User_T>
MBOOL IOPolicy<User_T>::isLoopBack(MUINT32 sensorID) const
{
  return mSensorPolicys.count(sensorID) ? mSensorPolicys.at(sensorID).mLoopBack : MFALSE;
}

template <typename User_T>
std::string IOPolicy<User_T>::toStr() const
{
  StringUtil str;
  str.printf("advSet(%d),noMdp(%d)\n", mUseAdvSetting, mNoMDP);
  for( const auto &sensor_map : mSensorPolicys )
  {
    MUINT32 sensor = sensor_map.first;
    str.printf("sensor[%d]:\n", sensor);
    for( const auto &path_map : sensor_map.second.mPathPoolMap )
    {
      for( const auto &id_map : path_map.second )
      {
        std::string ids = id_map.first.toStr();
        std::string attrs = id_map.second.mAttr.toStr();
        str.printf("path(%s), id(%s), pool(%p), attr(%s)\n",
                    User_T::toPathName(path_map.first), ids.c_str(), id_map.second.mPool.get(), attrs.c_str());
      }
    }
  }
  const char *outS = str.c_str();
  return std::string(outS != NULL ? outS : "unknown");
}

template <typename User_T>
typename IOPolicy<User_T>::PathID_T IOPolicy<User_T>::getAvailablePath(MUINT32 sensorID, typename IOPolicy<User_T>::PathID_T pathID) const
{
  return (mSensorPolicys.count(sensorID) && mSensorPolicys.at(sensorID).mPathPoolMap.count(pathID))
            ? pathID : User_T::DefaultPathID_ID;
}

template <typename User_T>
MUINT32 IOPolicy<User_T>::getInputMapCount(MUINT32 sensorID) const
{
    return mSensorPolicys.count(sensorID) ? mSensorPolicys.at(sensorID).mPathPoolMap.size() : 0;
}

template <typename User_T>
const typename IOPolicy<User_T>::NPoolMap& IOPolicy<User_T>::getInputMap(MUINT32 sensorID, typename IOPolicy<User_T>::PathID_T path) const
{
  static const PathID_T defaultID = User_T::DefaultPathID_ID;
  if( mSensorPolicys.count(sensorID) )
  {
    const SensorPolicy &sPolicy = mSensorPolicys.at(sensorID);
    return sPolicy.mPathPoolMap.count(path) ? sPolicy.mPathPoolMap.at(path)
            : sPolicy.mPathPoolMap.count(defaultID) ? sPolicy.mPathPoolMap.at(defaultID)
            : mDummyPoolMap;
  }
  else
  {
    return mDummyPoolMap;
  }
}

template <typename User_T>
IOPolicyType IOPolicy<User_T>::calPolicyType(PathID_T path, MUINT32 sensorID) const
{
  IOPolicyType out = IOPOLICY_INOUT;
  // NOTE currently these policy type are mutually exclusive
  if( isNoMDP() )                                   out = IOPOLICY_INOUT_NEXT;
  else if( getInputMap(sensorID, path).size() )     out = IOPOLICY_INOUT_EXCLUSIVE;
  else if( isLoopBack(sensorID) )                   out = IOPOLICY_LOOPBACK;
  return out;
}


template <typename Node_T, typename ReqInfo_T, typename User_T>
IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::NextInfo(const Node_T *node, MUINT32 pathCnt)
  : mNode(node)
  , mInPathCnt(pathCnt)
{
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
void IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::addType(const OutputType &type)
{
  mOutput.insert(type);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
void IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::addNext(Node_T *next, PathID_T path)
{
  mNexts.insert(NodePath(next, path));
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
bool IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::hasNext(Node_T *next, PathID_T path) const
{
  return mNexts.count(NodePath(next, path));
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
void IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::setTargetNode(TargetID_T target, Node_T *next)
{
  mTargets[target] = next;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
bool IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::isTargetOccupied(TargetID_T target, Node_T *next) const
{
  bool occupied = false;
  if( mTargets.count(target) )
  {
    Node_T *o  = mTargets.at(target);
    occupied = (o != NULL) && (o != next);
  }
  return occupied;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
void IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::addInputPath(PathID_T pathInPolicy, TargetID_T targetPath)
{
  mInput[pathInPolicy] = targetPath;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
bool IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::hasInput(PathID_T pathInPolicy) const
{
  return mInput.count(pathInPolicy);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IOControl<Node_T, ReqInfo_T, User_T>::TargetID_T
IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::getInput(PathID_T pathInPolicy) const
{
  return mInput.at(pathInPolicy);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
bool IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::needType(const OutputType &type) const
{
  return mOutput.count(type);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
void IOControl<Node_T, ReqInfo_T, User_T>::NextInfo::print(const char *path) const
{
  TRACE_FUNC_ENTER();
  const char *name = getName(mNode);
  StringUtil typeStr, nextStr, prevStr, tarStr;
  for( const auto &out : mOutput )
  {
    typeStr.printf("%s,", toName(out));
  }
  for( const NodePath &it : mNexts)
  {
    nextStr.printf("[%s,%s], ", getName(it.mNode), User_T::toPathName(it.mPath));
  }
  for( const auto &it : mInput )
  {
    prevStr.printf("[%s,%s]", User_T::toPathName(it.first), User_T::toTargetName(it.second));
  }
  for( const auto &it : mTargets )
  {
    tarStr.printf("[%s,%s]", User_T::toTargetName(it.first), getName(it.second));
  }

  CAM_ULOGD(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "path(%s) node(%s) inCnt(%d): outTypes(%s), Out NextBuf to nextNodes={%s}, Input={%s}, Target={%s}",
          path, name, mInPathCnt, typeStr.c_str(), nextStr.c_str(), prevStr.c_str(), tarStr.c_str());

  TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
IOControl<Node_T, ReqInfo_T, User_T>::IOControl()
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
IOControl<Node_T, ReqInfo_T, User_T>::~IOControl()
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::setRoot(Node_T *root)
{
    mRoot = root;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::addStream(const StreamType &stream, const NodeList &nodes)
{
  TRACE_FUNC_ENTER();
  if( mStreamPathMap.count(stream) )
  {
    MY_LOGW("StreamMap[%s] redefined", toName(stream));
  }
  mStreamPathMap[stream] = nodes;
  for( Node_T *node : nodes )
  {
    mAllNodeSet.insert(node);
  }
  TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IOControl<Node_T, ReqInfo_T, User_T>::NextPool
IOControl<Node_T, ReqInfo_T, User_T>::getFirstNextPool(const PolicyMap &policyMap, MUINT32 sensorID, Node_T *node,
                                                        typename IOControl<Node_T, ReqInfo_T, User_T>::PathID_T pathID) const
{
  NextPool out;
  if( policyMap.count(node) )
  {
    const NPoolMap &poolMap = policyMap.at(node).getInputMap(sensorID, pathID);
    if( poolMap.size() )
    {
      out = poolMap.begin()->second;
    }
  }
  return out;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::prepareMap(const std::map<MUINT32, StreamSet> &streams,
                                               const ReqInfo_T &reqInfo,
                                               std::unordered_map<MUINT32, IORequest> &reqMap)
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC("{%s} stream(%zu) sIDs(%zu)", reqInfo.dump(), streams.size(), sensorIDs.size());
  MBOOL ret = MTRUE;
  PolicyMap policyMap;
  preparePolicyInfos(reqInfo, policyMap);

  for( const auto &it : streams )
  {
    MUINT32 sID = it.first;
    const StreamSet &streamSet = it.second;
    IORequest &req = reqMap[sID]; // create IORequest
    req.mStreamSet = streamSet;
    for( StreamType s : streamSet )
    {
      prepareStreamMap(sID, s, policyMap, req);
    }

    mergeBufMap(req.mBufferTable);
  }


  TRACE_FUNC_EXIT();
  return ret;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::preparePolicyInfos(const ReqInfo_T &reqInfo, PolicyMap &policyMap) const
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MTRUE;
  for( const Node_T * node : mAllNodeSet )
  {
    policyMap[node] = std::move(node->getIOPolicy(reqInfo));
  }
  TRACE_FUNC_EXIT();
  return ret;
}


template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::dump(const std::unordered_map<MUINT32, IORequest> &reqMap) const
{
    for( const auto& it : reqMap )
    {
        CAM_ULOGD(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "IOReq Map Sensor(%d)", it.first);
        dump(it.second);
    }
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::dump(const IORequest &req) const
{
    TRACE_FUNC_ENTER();
    dumpInfo(req.mNextTable, req.mStreamSet);
    dumpInfo(req.mBufferTable);
    TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::dumpInfo(const NextTable &nextTable,  const StreamSet &streamSet) const
{
    TRACE_FUNC_ENTER();
    for( const auto &stream : mStreamPathMap )
    {
        if( streamSet.count(stream.first) )
        {
            for( const auto &node : stream.second)
            {
                auto out = nextTable.find(node);
                if( out != nextTable.end() )
                {
                    out->second.print(toName(stream.first));
                }
            }
        }
    }
    TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::dumpInfo(const BufferTable &bufferTable) const
{
    TRACE_FUNC_ENTER();
    for( const auto &it : bufferTable)
    {
        const char *nodeName = getName(it.first);
        for( const auto &pathBuf : it.second.mMap )
        {
            const char *path = User_T::toTargetName(pathBuf.first);
            for( const auto &buf : pathBuf.second )
            {
                std::string keyStr = buf.first.toStr();
                std::string attrStr = buf.second.mAttr.toStr();
                CAM_ULOGD(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "node(%s) targetID(%s) has buffer [%s], buf(%p), %s",
                          nodeName, path, keyStr.c_str(), buf.second.mBuffer.get(), attrStr.c_str());
            }
        }
        unsigned ind = 0;
        for( const auto &buf : it.second.mList )
        {
            std::string idStr = buf.mID.toStr();
            std::string attrStr = buf.mImg.mAttr.toStr();
            StringUtil keyStr;
            for( const typename NextImgReq::User &usr : buf.mUsers )
            {
                std::string s = usr.mID.toStr();
                keyStr.printf("[%s, (%s)]", User_T::toTargetName(usr.mTargetID), s.c_str());
            }
            CAM_ULOGD(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "node(%s) mList buffer[%d] id[%s], buf(%p), %s, map to {%s}",
                        nodeName, ind, idStr.c_str(), buf.mImg.mBuffer.get(), attrStr.c_str(), keyStr.c_str());
            ind++;
        }
    }
    TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::prepareStreamMap(MUINT32 sensorID, const StreamType &stream, const PolicyMap &policyMap, IORequest &req)
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  PathID_T pathID = User_T::toPathID(stream);
  PolicyInfoList policys = queryPathPolicy(sensorID, stream, policyMap, pathID);
  ret = (stream == STREAMTYPE_ASYNC) ?
        prepareAsyncNext(sensorID, policyMap, policys, pathID, req.mAsyncNext, req.mAsyncBuffer) :
        backwardCalc(sensorID, stream, policyMap, policys, pathID, req);
  TRACE_FUNC_EXIT();
  return ret;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::prepareAsyncNext(MUINT32 sensorID, const PolicyMap &policyMap, const PolicyInfoList &policys, PathID_T pathID, Node_T* &asyncNext, NextBuf &asyncBuf)
{
  TRACE_FUNC_ENTER();
  for( const PolicyInfo &info : policys )
  {
    if( info.mNode != NULL && info.mRun )
    {
      asyncNext = info.mNode;
      NextPool nPool = getFirstNextPool(policyMap, sensorID, info.mNode, pathID);
      if( nPool.mPool )
      {
        asyncBuf.mBuffer = nPool.mPool->requestIIBuffer();
        asyncBuf.mAttr = nPool.mAttr;
      }
      else
      {
        CAM_ULOGW(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "Async Next Buffer N_IMG_NORMAL pool is NULL!, async buffer request FAIL");
      }
      break;
    }
  }
  TRACE_FUNC_EXIT();
  return MTRUE;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IOControl<Node_T, ReqInfo_T, User_T>::PolicyInfoList
IOControl<Node_T, ReqInfo_T, User_T>::queryPathPolicy(MUINT32 sensorID, const StreamType &stream, const PolicyMap &policyMap, PathID_T pathID) const
{
  TRACE_FUNC_ENTER();
  PolicyInfoList list;
  auto path = mStreamPathMap.find(stream);
  if( path != mStreamPathMap.end() )
  {
    for( Node_T *node : path->second )
    {
      if( !node || !policyMap.count(node) )
      {
        CAM_ULOGW(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream=%s Invalid node=%p or policyMap has no this node, count(%zu)",
                  toName(stream), node, policyMap.count(node));
        continue;
      }
      const IOPolicy &policy = policyMap.at(node);
      list.emplace_back(PolicyInfo(node,
                                   policy.isRun(stream, sensorID),
                                   policy.getInputMapCount(sensorID),
                                   policy.calPolicyType(pathID, sensorID)));
    }
  }
  TRACE_FUNC_EXIT();
  return list;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::appendNextBufMap(const Node_T *node,
                                                            const typename IOControl<Node_T, ReqInfo_T, User_T>::NPoolMap &poolMap,
                                                            typename IOControl<Node_T, ReqInfo_T, User_T>::NextBufMap &outMap) const
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MTRUE;
  TRACE_FUNC("node(%s) alloc next buffer", getName(node));
  for( const auto &it : poolMap )
  {
    NextBuf img;
    if( !it.second.isValid() )
    {
        CAM_ULOGE(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "node(%s) needBuf(%d), pool(%p) not match!!",
                  getName(node), img.mAttr.needBuffer(), it.second.mPool.get());
    }
    else if ( it.second.mPool != NULL )
    {
        img.mBuffer = it.second.mPool->requestIIBuffer();
    }
    img.mAttr = it.second.mAttr;
    if( outMap.count(it.first) )
    {
        std::string keyStr = it.first.toStr();
        CAM_ULOGE(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "node(%s) buffer id [%s] already exist buffer map!!",
                  getName(node), keyStr.c_str());
        ret = MFALSE;
    }
    outMap[it.first] = img;
  }
  TRACE_FUNC_EXIT();
  return ret;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IOControl<Node_T, ReqInfo_T, User_T>::NextImgReqList
IOControl<Node_T, ReqInfo_T, User_T>::toList(const NextBufMap &map, TargetID_T targetID) const
{
  NextImgReqList list;
  for( const auto &it : map )
  {
    list.push_back({ .mID = it.first, .mImg = it.second, .mUsers = {typename NextImgReq::User(targetID, it.first)} });
  }
  return list;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::tryMerge(TargetID_T targetID, NextImgReqList &list, size_t searchRange, const NextID_T &id, NextBuf &img) const
{
    size_t i = 0;
    bool find = false;
    for( NextImgReq &it : list )
    {
        if( i >= searchRange )
        {
            break;
        }
        if( it.mID.canMerge(id) && it.mImg.mAttr == img.mAttr )
        {
            img.mBuffer = it.mImg.mBuffer;
            it.mUsers.push_back(typename NextImgReq::User(targetID,id));
            find = true;
            break;
        }
        i++;
    }

    if( !find )
    {
        list.push_back({ .mID = id, .mImg = img, .mUsers = {typename NextImgReq::User(targetID, id)} });
    }
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IOControl<Node_T, ReqInfo_T, User_T>::mergeBufMap(BufferTable &bufferTable) const
{
  TRACE_FUNC_ENTER();
  for( auto &it : bufferTable )
  {
    MBOOL pushed = MFALSE;
    NextBufMap *refMap = NULL;
    size_t searchRange = 0;
    for( auto &pathMap : it.second.mMap )
    {
        if( !pushed )
        {
            pushed = MTRUE;
            refMap = &pathMap.second;
            it.second.mList = toList(pathMap.second, pathMap.first);
        }
        else
        {
            for( auto &bufIt : pathMap.second )
            {
                tryMerge(pathMap.first, it.second.mList, searchRange, bufIt.first, bufIt.second);
            }
        }
        searchRange = it.second.mList.size();
    }
  }
  TRACE_FUNC_EXIT();
}


static inline bool useNext(const OutputType &type, const IOPolicyType &policy)
{
  return (type == OUTPUT_NEXT_FULL && policy == IOPOLICY_INOUT_NEXT);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IOControl<Node_T, ReqInfo_T, User_T>::backwardCalc(MUINT32 sensorID,
                                                 const StreamType &stream,
                                                 const PolicyMap &policyMap,
                                                 const PolicyInfoList &policyList,
                                                 PathID_T pathID,
                                                 IORequest &req) const
{
    TRACE_FUNC_ENTER();
    int count = 0;
    MBOOL ret = MTRUE;
    IOPolicy dummyPolicy;
    Node_T *nextNode = NULL;
    OutputType nextType = streamToType(stream);
    // TODO TargetID mapping by user define, now just use static mappingKey
    TargetID_T targetID = User_T::toTargetID(pathID);

    TRACE_FUNC("sID(%d) type(%s)", sensorID, toName(nextType));
    for( auto it = policyList.crbegin(), end = policyList.crend(); it != end; ++it )
    {
        Node_T *node = it->mNode;
        IOPolicyType policy = it->mRun ? it->mPolicyType : IOPOLICY_BYPASS;
        TRACE_FUNC("node(%s)[%s][%s] next(%s)[%s]",
                getName(node), toName(policy), getName(nextNode), toName(nextType));
        if( node != NULL && policy != IOPOLICY_BYPASS )
        {
            if( !req.mNextTable.count(node) )
            {
                req.mNextTable[node] = NextInfo(node, it->mAvailablePathCnt);
            }
            NextInfo &info = req.mNextTable[node];
            info.addType(nextType);
            if( policy == IOPOLICY_LOOPBACK )
            {
                info.addType(OUTPUT_FULL);
            }
            const IOPolicy &nextPolicy = policyMap.count(nextNode) ? policyMap.at(nextNode) : dummyPolicy;
            PathID_T pathInPolicy = nextPolicy.getAvailablePath(sensorID, pathID);
            if( nextType == OUTPUT_NEXT_FULL && !info.hasNext(nextNode, pathInPolicy) )
            {
                info.addNext(nextNode, pathInPolicy);
                NextInfo &nextInfo = req.mNextTable[nextNode];
                if( nextInfo.hasInput(pathInPolicy) && nextInfo.getInput(pathInPolicy) != targetID)
                {
                    dump(req);
                    CAM_ULOG_FATAL(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream(%s) nextnode(%s)'s pathInPolicy(%s) buffers already output by other targetID."
                            "  1 PathID_T buffer output by 2 more target path is not valid!!!!",
                            toName(stream), getName(nextNode), User_T::toPathName(pathInPolicy));
                }
                if( info.isTargetOccupied(targetID, nextNode) )
                {
                    dump(req);
                    CAM_ULOG_FATAL(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream(%s) targetID(%s) already occupied by other node rather than current nextnode(%s)'s "
                            "  1 PathID_T buffer output by 2 more target path is not valid!!!!",
                            toName(stream), User_T::toTargetName(targetID), getName(nextNode));
                }
#if 0
                if( req.mBufferTable[node].mMap.count(targetID) ) // need check --> no need, if A out B default+tiny, it is alright.
                {
                    dump(req);
                    CAM_ULOG_FATAL(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream(%s) node(%s) pathID(%s) already exist, new node(%s) buf will dropped!! ERROR!",
                            toName(stream), getName(node), User_T::toPathName(pathInPolicy), getName(nextNode));
                }
#endif
                info.setTargetNode(targetID, nextNode);
                nextInfo.addInputPath(pathInPolicy, targetID);
                if( !appendNextBufMap(nextNode, nextPolicy.getInputMap(sensorID, pathInPolicy), req.mBufferTable[node].mMap[targetID]) )
                {
                    dump(req);
                    CAM_ULOG_FATAL(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream(%s) node(%s) pathID(%s) from nextnode(%s), append buffer failed!  ERROR!",
                            toName(stream), getName(node), User_T::toPathName(pathInPolicy), getName(nextNode));
                }
                if( policy == IOPOLICY_INOUT_NEXT && req.mBufferTable[node].mMap[targetID].size() > 1)
                {
                    CAM_ULOG_FATAL(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "stream(%s) node(%s) has no mdp, but Next Buf size(%zu) > 1 !!! ERROR!",
                        toName(stream), getName(node), req.mBufferTable[node].mMap[targetID].size());
                }
            }

            nextNode = useNext(nextType, policy) ? nextNode : node;
            switch( policy )
            {
            case IOPOLICY_INOUT_EXCLUSIVE:  nextType = OUTPUT_NEXT_FULL;  break;
            case IOPOLICY_INOUT_NEXT:       nextType = OUTPUT_NEXT_FULL;  break;
            default:                        nextType = OUTPUT_FULL;       break;
            }

            ++count;
        }
    }


  if( !count )
  {
    CAM_ULOGW(NSCam::Utils::ULog::MOD_FPIPE_COMMON, "sID(%d) Can not found active policy in path(%s)!", sensorID, toName(stream));
    ret = MFALSE;
  }
  TRACE_FUNC_EXIT();
  return MFALSE;
}


template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::isFullBufHold() const
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  for( const auto &bufferMap : mBufferTable )
  {
    if( bufferMap.second.mList.size() )
    {
      ret = MTRUE;
      break;
    }
  }
  TRACE_FUNC_EXIT();
  return ret;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needPreview(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_STREAM_PREVIEW);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needPreviewCallback(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_STREAM_PREVIEW_CALLBACK);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needRecord(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_STREAM_RECORD);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needFD(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_STREAM_FD);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needPhysicalOut(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_STREAM_PHYSICAL);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needFull(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_FULL);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needNextFull(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return needOutputType(node, OUTPUT_NEXT_FULL);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needAsync(const Node_T *node) const
{
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return mAsyncNext != NULL && needPreview(node);
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IORequest<Node_T, ReqInfo_T, User_T>::NextCollection IORequest<Node_T, ReqInfo_T, User_T>::getNextFullImg(const Node_T *node)
{
  TRACE_FUNC_ENTER();
  NextCollection out = std::move(mBufferTable[node]);
  mBufferTable[node] = NextCollection();
  TRACE_FUNC_EXIT();
  return out;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
typename IORequest<Node_T, ReqInfo_T, User_T>::NextBuf IORequest<Node_T, ReqInfo_T, User_T>::getAsyncImg(const Node_T *node)
{
  (void)node;
  TRACE_FUNC_ENTER();
  TRACE_FUNC_EXIT();
  return mAsyncBuffer;
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MVOID IORequest<Node_T, ReqInfo_T, User_T>::clearAsyncImg(const Node_T *node)
{
  (void)node;
  TRACE_FUNC_ENTER();
  mAsyncBuffer.mBuffer = NULL;
  TRACE_FUNC_EXIT();
}

template <typename Node_T, typename ReqInfo_T, typename User_T>
MBOOL IORequest<Node_T, ReqInfo_T, User_T>::needOutputType(const Node_T *node, const OutputType &type) const
{
  TRACE_FUNC_ENTER();
  MBOOL ret = MFALSE;
  const auto &entry = mNextTable.find(node);
  ret = entry != mNextTable.end() &&
        entry->second.needType(type);
  TRACE_FUNC_EXIT();
  return ret;
}

} // namespace NSFeaturePipe
} // namespace NSCamFeature
} // namespace NSCam

#include "PipeLogHeaderEnd.h"

#endif // _MTK_CAMERA_FEATURE_PIPE_CORE_IO_UTIL_H_
