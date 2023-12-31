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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PIPELINE_IPIPELINENODE_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PIPELINE_IPIPELINENODE_H_
//
#include <utils/KeyedVector.h>
#include <utils/Printer.h>
#include <utils/RefBase.h>
//
#include <mtkcam/def/common.h>
#include <mtkcam3/pipeline/stream/IStreamBufferSet.h>
#include <mtkcam3/pipeline/stream/IStreamInfo.h>
#include "IPipelineDAG.h"
#include "types.h"

#include <mtkcam/utils/std/ULog.h>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {


/******************************************************************************
 *
 ******************************************************************************/
class IPipelineNodeMap;
class IPipelineFrame;
class IPipelineNode;


/**
 * Type of Camera Pipeline Node Id.
 */
using Pipeline_NodeId_T = NSCam::v3::pipeline::NodeId_T;


/**
 * An interface of Pipeline node map (key:NodeId_T, value:NodePtr_T).
 */
class IPipelineNodeMap
    : public virtual android::RefBase
{
public:     ////                    Definitions.
    typedef Pipeline_NodeId_T       NodeId_T;
    typedef IPipelineNode           NodeT;
    typedef android::sp<NodeT>      NodePtrT;

public:     ////                    Operations.
    virtual MBOOL                   isEmpty() const                         = 0;
    virtual size_t                  size() const                            = 0;

    virtual NodePtrT                nodeFor(NodeId_T const& id) const       = 0;
    virtual NodePtrT                nodeAt(size_t index) const              = 0;

};


/**
 * An interface of (in-flight) pipeline frame listener.
 */
class IPipelineFrameListener
    : public virtual android::RefBase
{
public:     ////                    Definitions.
    typedef Pipeline_NodeId_T       NodeId_T;

    enum
    {
        /** This frame is released */
        eMSG_FRAME_RELEASED,

        /** All output meta buffers released */
        eMSG_ALL_OUT_META_BUFFERS_RELEASED,

        /** All output image buffers released */
        eMSG_ALL_OUT_IMAGE_BUFFERS_RELEASED,

    };

    /**
     * Invoked when something happens.
     *
     * @param[in] frameNo: frame number.
     *
     * @param[in] message: a message to indicate what happen.
     *
     * @param[in] pCookie: the listener's cookie.
     */
    virtual MVOID                   onPipelineFrame(
                                        MUINT32 const frameNo,
                                        MUINT32 const message,
                                        MVOID*const pCookie
                                    )                                       = 0;

    /**
     * Invoked when something happens.
     *
     * @param[in] frameNo: frame number.
     *
     * @param[in] nodeId: node ID.
     *
     * @param[in] message: a message to indicate what happen.
     *
     * @param[in] pCookie: the listener's cookie.
     */
    virtual MVOID                   onPipelineFrame(
                                        MUINT32 const frameNo,
                                        NodeId_T const nodeId,
                                        MUINT32 const message,
                                        MVOID*const pCookie
                                    )                                       = 0;

};


struct IPipelineNodeCallback;
/**
 * An interface of (in-flight) pipeline frame.
 */
class IPipelineFrame
    : public virtual android::RefBase
{
public:     ////                    Definitions.
    typedef Pipeline_NodeId_T       NodeId_T;
    using GroupFrameTypeT = NSCam::v3::pipeline::NSPipelineContext::GroupFrameType;
    using IPipelineDAG = NSCam::v3::pipeline::IPipelineDAG;

    struct ImageInfoIOMap
    {
        android::DefaultKeyedVector<StreamId_T, android::sp<IImageStreamInfo> > vIn;
        android::DefaultKeyedVector<StreamId_T, android::sp<IImageStreamInfo> > vOut;
        MBOOL       isPhysical = false;
    };

    struct MetaInfoIOMap
    {
        android::DefaultKeyedVector<StreamId_T, android::sp<IMetaStreamInfo> >  vIn;
        android::DefaultKeyedVector<StreamId_T, android::sp<IMetaStreamInfo> >  vOut;
        MBOOL       isPhysical = false;
    };

    struct ImageInfoIOMapSet
        : public android::Vector<ImageInfoIOMap>
    {
    };

    struct MetaInfoIOMapSet
        : public android::Vector<MetaInfoIOMap>
    {
    };

    struct InfoIOMapSet
    {
        typedef IPipelineFrame::ImageInfoIOMapSet   ImageInfoIOMapSet;
        typedef IPipelineFrame::MetaInfoIOMapSet    MetaInfoIOMapSet;
        ImageInfoIOMapSet           mImageInfoIOMapSet;
        MetaInfoIOMapSet            mMetaInfoIOMapSet;
    };

    struct BatchInfo
    {
        size_t batchSize = 0;
        size_t index = 0;
    };

public:     ////                    Operations.
    /**
     * Return a pointer to a null-terminated string to indicate a magic name.
     */
    virtual char const*             getMagicName() const                    = 0;

    /**
     * Return a magic instance. Usually it is used for downcasting.
     */
    virtual void*                   getMagicInstance() const                = 0;

public:     ////                    Operations.

    virtual MUINT32                 getFrameNo() const                      = 0;
    virtual MUINT32                 getRequestNo() const                    = 0;
    virtual void                    getBatchInfo(BatchInfo &out) const      = 0;
    virtual void                    setBatchInfo(BatchInfo const& arg)      = 0;
    virtual MBOOL                   IsReprocessFrame() const                = 0;

    virtual auto    getGroupFrameType() const -> GroupFrameTypeT            = 0;

    virtual android::sp<IPipelineNodeMap const>
                                    getPipelineNodeMap() const              = 0;
    virtual IPipelineDAG const&     getPipelineDAG() const                  = 0;
    virtual android::sp<IPipelineDAG>     getPipelineDAGSp()                = 0;
    virtual IStreamBufferSet&       getStreamBufferSet() const              = 0;
    virtual IStreamInfoSet const&   getStreamInfoSet() const                = 0;
    /**
     * Note: getPipelineNodeCallback() const
     * actually, IPipelineNodeCallback is stord as wp. Calling this
     * function, this module will help to promote IPipelineNodeCallback from wp
     * to sp.
     */
    virtual android::sp<IPipelineNodeCallback>
                                    getPipelineNodeCallback() const         = 0;

    virtual MERROR                  queryIOStreamInfoSet(
                                        NodeId_T const& nodeId,
                                        android::sp<IStreamInfoSet const>& rIn,
                                        android::sp<IStreamInfoSet const>& rOut
                                    ) const                                 = 0;

    virtual MERROR                  queryInfoIOMapSet(
                                        NodeId_T const& nodeId,
                                        InfoIOMapSet& rIOMapSet
                                    ) const                                 = 0;

    /**
     * true indicates this frame has been (being) aborted.
     */
    virtual auto    isAborted() const -> bool                               = 0;

    /**
     * true indicates this frame is expected to be successfully processed
     * and unexpected to be aborted even during IPipelineNode::flush().
     */
    virtual auto    isUnexpectedToAbort() const -> bool                     = 0;

    /**
     * Try to get the sensor timestamp.
     *
     * Definition:
     * https://developer.android.com/reference/android/hardware/camera2/CaptureResult#SENSOR_TIMESTAMP
     *
     * getSensorTimestamp returns a valid sensor timestamp only if the timestamp is ready.
     *
     * @return
     *      0 indicates an invalid sensor timestampe
     *      non-zero indicates a valid sensor timestampe
     */
    virtual auto    getSensorTimestamp() const -> int64_t                   = 0;
    virtual auto    setSensorTimestamp(
                        int64_t timestamp,
                        char const* callerName = ""
                    ) -> int                                                = 0;

    /**
     * Try to get the active physical ID
     *
     * Definition:
     * https://developer.android.com/reference/android/hardware/camera2/CaptureResult#LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID
     *
     * getActivePhysicalId returns the active physical id when it was set
     *
     * @return
     *      -1 indicates an invalid active physical ID
     *      others: valid active physical ID before remapping
     */
    virtual auto    getActivePhysicalID() const -> int32_t                   = 0;
    virtual auto    setActivePhysicalID(
                        int32_t physicalId
                    ) -> int                                                 = 0;

    /**
     * Attach a pipeline frame listener.
     *
     * @param[in] pListener: the listener to attach.
     *
     * @param[in] pCookie: the listener's cookie.
     *
     * @return
     *      0 indicates success; otherwise failure.
     */
    virtual MERROR                  attachListener(
                                        android::wp<IPipelineFrameListener>const& pListener,
                                        MVOID* pCookie
                                    )                                       = 0;

    /**
     * Dump debugging state.
     */
    virtual MVOID                   dumpState(
                                        android::Printer& printer,
                                        const std::vector<std::string>& options
                                    ) const                                 = 0;

};

/**
 * An interface of callback function from node to pipeline context
 */
class INodeCallbackToPipeline
    : public virtual android::RefBase
{
    public:     ////                    Definitions.
        enum eNoticeType
        {
            eNotice_ReadyToEnque,
        };

        struct  CallBackParams
        {
            /**
             * A unique value for node id.
             */
            Pipeline_NodeId_T       nodeId;

            /**
             * Last frame number of node to process
             */
            MUINT32                 lastFrameNum;

            /**
             * callback notice type
             */
            eNoticeType             noticeType;

            CallBackParams()
                : nodeId(0)
                , lastFrameNum(0)
                , noticeType(eNotice_ReadyToEnque)
            {}
        };

    public:    ////                    Attributes.
        virtual MVOID                   onCallback(
                                            CallBackParams    param
                                        )                                   = 0;
        virtual MVOID                   onHWReady()                         = 0;
};

/**
 * An interface of pipeline node.
 */

class IPipelineNode
    : public virtual android::RefBase
{
public:     ////                    Definitions.
    typedef Pipeline_NodeId_T       NodeId_T;
    typedef unsigned int            ModuleId;

    /**
     * Initialization Parameters.
     */
    struct  InitParams
    {
        /**
         * An index to indicate which camera device to open.
         */
        MINT32                      openId = 0;

        /**
         * A unique value for the node id.
         */
        NodeId_T                    nodeId = 0;

        /**
         * A pointer to a null-terminated string for the node name.
         */
        char const*                 nodeName = NULL;

        /**
         * index list to indicate which camera devices cooperate with this opened camera device.
         */
        android::Vector<MUINT32>    subOpenIdList;

    };

public:     ////                    Attributes.
    /**
     * @return
     *      An index to indicate which camera device to open.
     */
    virtual MINT32                  getOpenId() const                       = 0;

    /**
     * @return
     *      A unique node id.
     */
    virtual NodeId_T                getNodeId() const                       = 0;

    /**
     * @return
     *      A null-terminated string for the node name.
     */
    virtual char const*             getNodeName() const                     = 0;

    virtual ModuleId                getULogModuleId()                     = 0;

public:     ////                    Operations.

    /**
     *
     */
    virtual MERROR                  init(InitParams const& rParams)         = 0;

    /**
     *
     */
    virtual MERROR                  uninit()                                = 0;

    /**
     *
     */
    struct TriggerDB
    {
        const char* msg = "";
        int         err = 0;
        bool        needDumpCallstack = false;
        bool        needTerminateCurrentProcess = false;
    };
    virtual MERROR                  triggerdb(TriggerDB const& arg)         = 0;

    /**
     *
     */
    virtual MERROR                  flush()                                 = 0;

    /**
     *
     */
    virtual MERROR                  flush(
                                        android::sp<IPipelineFrame> const &pFrame
                                    )                                       = 0;

    /**
     *
     */
    virtual MERROR                  kick()                                  = 0;

    /**
     *
     */
    virtual MERROR                  setNodeCallBack(
                                        android::wp<INodeCallbackToPipeline> pCallback
                                    )                                       = 0;

    /**
     *
     */
    virtual MERROR                  queue(
                                        android::sp<IPipelineFrame> pFrame
                                    )                                       = 0;

    /**
     *
     */
    virtual std::string             getStatus()                             = 0;

    /**
     *
     */
    virtual void                    preConfig(int32_t cmd, void* pData)     = 0;
};


/**
 * An interface of callback function from node to pipeline user
 */
struct IPipelineNodeCallback : public virtual android::RefBase
{
public:
    enum eCtrlType
    {
        eCtrl_Setting,
        eCtrl_Sync,
        eCtrl_Resize,
        eCtrl_Readout,
    };
public:
    virtual MVOID                   onDispatchFrame(
                                        android::sp<IPipelineFrame> const& pFrame,
                                        Pipeline_NodeId_T nodeId
                                    )                                       = 0;

    /**
     * This method is called when some (but not all) result metadata are available.
     *
     * For each frame, some result metadata might be available earlier than others.
     * For performance-oriented use-cases, applications should query the metadata they need to make
     * forward progress from the partial results and avoid waiting for the completed result.
     */
    struct MetaResultAvailable
    {
        /**
         * @param (Partial) result metadata.
         */
        IMetadata const*            resultMetadata = nullptr;

        /**
         * @param The pipeline frame which is associating with the result metadata.
         */
        android::sp<IPipelineFrame> frame = nullptr;

        /**
         * @param The node id which performs this callback.
         */
        Pipeline_NodeId_T           nodeId{0};

        /**
         * @param The caller name (for debugging).
         */
        std::string                 callerName;
    };
    virtual auto    onMetaResultAvailable(
                      MetaResultAvailable&& arg
                    ) -> void                                               = 0;

    virtual MVOID                   onEarlyCallback(
                                        MUINT32           requestNo,
                                        Pipeline_NodeId_T nodeId,
                                        StreamId_T        streamId,
                                        IMetadata const&  rMetaData,
                                        MBOOL             errorResult = MFALSE
                                    )                                       = 0;
    // Control-Callback
    virtual MVOID                   onCtrlSetting(
                                        MUINT32             requestNo,
                                        Pipeline_NodeId_T   nodeId,
                                        StreamId_T const    metaAppStreamId,
                                        IMetadata&          rAppMetaData,
                                        StreamId_T const    metaHalStreamId,
                                        IMetadata&          rHalMetaData,
                                        MBOOL&              rIsChanged
                                    )                                       = 0;
    virtual MVOID                   onCtrlSync(
                                        MUINT32             equestNo,
                                        Pipeline_NodeId_T   nodeId,
                                        MUINT32             index,
                                        MUINT32             type,
                                        MINT64              duration
                                    )                                       = 0;
    virtual MVOID                   onCtrlResize(
                                        MUINT32             requestNo,
                                        Pipeline_NodeId_T   nodeId,
                                        StreamId_T const    metaAppStreamId,
                                        IMetadata&          rAppMetaData,
                                        StreamId_T const    metaHalStreamId,
                                        IMetadata&          rHalMetaData,
                                        MBOOL&              rIsChanged
                                    )                                       = 0;
    virtual MVOID                   onCtrlReadout(
                                        MUINT32             requestNo,
                                        Pipeline_NodeId_T   nodeId,
                                        StreamId_T const    metaAppStreamId,
                                        IMetadata&          rAppMetaData,
                                        StreamId_T const    metaHalStreamId,
                                        IMetadata&          rHalMetaData,
                                        MBOOL&              rIsChanged
                                    )                                       = 0;
    virtual MBOOL                   needCtrlCb(
                                        eCtrlType           eType
                                    )                                       = 0;
    // for Fast S2S callback
    virtual MVOID                   onNextCaptureCallBack(
                                        MUINT32             requestNo,
                                        Pipeline_NodeId_T   nodeId,
                                        MUINT32             requestCnt,
                                        MBOOL               bSkipCheck
                                    )                                       = 0;

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_PIPELINE_PIPELINE_IPIPELINENODE_H_

