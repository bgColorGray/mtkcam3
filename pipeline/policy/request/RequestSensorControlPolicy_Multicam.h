/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM_PIPELINE_POLICY_REQUESTSENSORCONTROLPOLICY_MULTICAM_H_
#define _MTK_HARDWARE_MTKCAM_PIPELINE_POLICY_REQUESTSENSORCONTROLPOLICY_MULTICAM_H_
//
#include <mtkcam/drv/IHalSensor.h>
#include <mtkcam/utils/hw/HwTransform.h>
#include "MyUtils.h"

//
#include <mtkcam3/pipeline/policy/IRequestSensorControlPolicy.h>
#include <mtkcam3/3rdparty/core/sensor_control_type.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace v3 {
namespace pipeline {
namespace policy {
namespace requestsensorcontrol {
class SensorControllerBehaviorManager;
class Hal3AObject
{
public:
    Hal3AObject(const std::vector<int32_t> &sensorId);
    ~Hal3AObject();
public:
    bool update();
    bool isAFDone(int32_t sensorId);
    MINT32 getAEIso(int32_t sensorId);
    MINT32 getAELv_x10(int32_t sensorId);
    bool is2ASyncReady(int32_t masterSensorId);
    bool isAFSyncDone(int32_t masterSensorId);
    bool isAaoReady(std::vector<uint32_t> const& sensorList);
    bool isFramesyncReady(int32_t masterSensorId);
    bool setSync2ASetting(int32_t masterSensorId, int32_t resumeSensorId);
private:
    std::unordered_map<int32_t, std::shared_ptr<NS3Av3::IHal3A> >
                    mvHal3A;
    std::unordered_map<int32_t, NS3Av3::DualZoomInfo_T>
                    mv3AInfo;
};
/******************************************************************************
 *
 ******************************************************************************/
class DeviceInfoHelper
{
public:
    DeviceInfoHelper(
        const int32_t &deviceId);
    ~DeviceInfoHelper();
public:
    bool getPhysicalSensorFov(
                    int32_t physicalSensorId,
                    float &h,
                    float &v);
    bool getPhysicalSensorActiveArray(
                    int32_t physicalSensorId,
                    MRect &activeArray);
    bool getPhysicalSensorFullSize(
                    int32_t physicalSensorId,
                    MSize &sensorSize);
    bool getPhysicalFocalLength(
                    int32_t physicalSensorId,
                    MFLOAT &focalLength);
    bool getLogicalFocalLength(
                    std::vector<MFLOAT>& focalLength);
private:
    auto    updatePhysicalFocalLength(
                    int32_t sensorId,
                    MFLOAT &focalLength,
                    IMetadata &sensorMetadata) -> bool;
    auto    updatePhysicalSensorFov(
                    int32_t sensorId,
                    MFLOAT &focalLength,
                    std::shared_ptr<NSCam::MSizeF>& pSensorFov,
                    IMetadata &sensorMetadata) -> bool;
private:
    std::unordered_map<int32_t, std::shared_ptr<NSCam::SensorStaticInfo> >
                    mvPhysicalSensorStaticInfo;
    std::unordered_map<int32_t, std::shared_ptr<MRect> >
                    mvPhysicalSensorActiveArray;
    std::unordered_map<int32_t, std::shared_ptr<MSizeF> >
                    mvPhysicalSensorFov;
    std::unordered_map<int32_t, MFLOAT >
                    mvPhysicalFocalLength;
    std::vector<MFLOAT> mvAvailableFocalLength;
};
/******************************************************************************
 *
 ******************************************************************************/
struct ParsedSensorStateResult
{
    // online/offline list
    std::vector<uint32_t> vNeedOnlineSensorList;
    std::vector<uint32_t> vNeedOfflineSensorList;
    // online state
    std::vector<uint32_t> vStandbySensorIdList;
    std::vector<uint32_t> vResumeSensorIdList;
    std::vector<uint32_t> vStreamingSensorIdList;
    std::vector<uint32_t> vGotoStandbySensorIdList;
    // store alternative crop region
    std::unordered_map<uint32_t, MRect> vAlternactiveCropRegion;
    std::unordered_map<uint32_t, MRect> vAlternactiveCropRegion_Cap;
    std::unordered_map<uint32_t, MRect> vAlternactiveCropRegion_3A;
    std::unordered_map<uint32_t, std::vector<MINT32>> vAlternactiveAfRegion;
    std::unordered_map<uint32_t, std::vector<MINT32>> vAlternactiveAeRegion;
    std::unordered_map<uint32_t, std::vector<MINT32>> vAlternactiveAwbRegion;
    // based on current 3rd decision output master id.
    int32_t masterId = -1;
    // previous request master id
    int32_t previousMasterId = -1;
    // quality flag
    MUINT32 qualityFlag = (MUINT32)-1;
};
/******************************************************************************
 *
 ******************************************************************************/
struct BehaviorUserData
{
    ParsedSensorStateResult* parsedResult = nullptr;
    uint32_t requestNo = 0;
    int32_t masterId = -1;
    std::vector<int32_t> vSensorId;
    SensorMap<uint32_t> vSensorMode;
    bool bNeedP2Capture = false;
    bool bNeedFusion = false;
    bool isSensorSwitchGoing = false;
    bool isNeedNotifySync3A = false;
    bool enableSyncAf = false;
    float fZoomRatio = 1.0f;
};
/******************************************************************************
 *
 ******************************************************************************/
class ISPQualityManager
{
public:
    ISPQualityManager(
                    int32_t openId,
                    uint32_t mLogLevel);
    ~ISPQualityManager();
public:
    bool init(
            SensorMap<P1HwSetting> const* pvP1HwSetting);
public:
    bool isSameToBaseQualityFlag();
    // If mTriggerBackToOri is true, it will alwasy return mOriIspQualityFlag.
    MUINT32 getQualityFlag(
                        uint32_t const&requestNo,
                        int32_t const&master,
                        int32_t const&slave);
    MUINT32 getBaseQualityFlag();
    bool updateBaseQualityFlag(
                        uint32_t const& requestNo,
                        int32_t const& master,
                        int32_t const& slave);
    bool revertQualityToBase();
    bool qualitySwitchDone();
    bool isQualitySwitchDone();
private:
    MUINT32 buildQualityFlag(
                        int32_t master,
                        int32_t slave,
                        bool supportQualitySwitch);
private:
    struct QualityFlag {
        uint32_t reqNo = 0;
        MUINT32 quality = 0;
    };
private:
    int32_t mOpenId = -1;
    uint32_t mLogLevel = 0;
    bool mSupportIspQualitySwitch = false;
    QualityFlag mBaseQualityFlag;
    QualityFlag mPrvQualityFlag;
    //
    bool bQualitySwitching = false;
    std::mutex mLock;
};
/******************************************************************************
 *
 ******************************************************************************/
class SensorStateManager
{
public:
    struct Input {
        uint32_t requestNo = 0;
        bool isCaptureing = false;
        bool bFirstReqCapture = false;
        SensorMap<P1HwSetting> const* pvP1HwSetting = nullptr;
    };
public:
    struct SensorGroupManager{
        SensorGroupManager();
        bool isSameGroup(uint32_t &val1, uint32_t &val2);
        // Set exclusive group by 3rd sensor control
        bool setGroupMap(SensorMap<int> const* pGroupMap);
        // <sensor id, group index>
        SensorMap<int> mSensorMap;
    };
public:
    SensorStateManager() = delete;
    SensorStateManager(
                        int32_t openId,
                        const std::vector<int32_t> &vSensorId,
                        uint32_t maxOnlineCount,
                        int32_t  initRequest,
                        uint32_t loglevel);
    ~SensorStateManager();
public:
    enum SensorStateType
    {
        E_ONLINE,
        E_OFFLINE,
        E_INVALID,
    };
private:
    struct SensorState
    {
        SensorStateType eSensorState = SensorStateType::E_ONLINE;
        sensorcontrol::SensorStatus eSensorOnlineStatus =
                sensorcontrol::SensorStatus::E_STREAMING;
        uint32_t iIdleCount = 0;
    };
public:
    /*
     * set exclusive group map in SensorGroupManager.
     */
    bool setExclusiveGroup(SensorMap<int> const* pGroupMap);
    /*
     * according SensorControlParamOut to update sensor status.
     */
    bool update(
            sensorcontrol::SensorControlParamOut const& out,
            ParsedSensorStateResult &result,
            Input const& inputParams);
    /*
     * notify sensor switch done to SensorStateManager.
     */
    void switchSensorDone();
    /*
     * query sensor switch is going or not.
     */
    bool isSensorSwitchGoing();
    /*
     * query sensor switch is OK or not.
     */
    bool isSensorOKforSwitch();
    /*
     * query is in initRequest flow or not
     */
    void switchQualityDone();
    bool isInitRequestGoing();
    void setSensorConfigureDone(bool flag);
    void setNextCaptureDone(bool flag);
    void setAfDone(bool flag);
    void setInitParsedSensorStateResult(uint32_t sensorId);
    void setInitMasterId(int32_t sensorId);
    /*
     * query sensor switch is OK or not.
     */
    void setInitReqCnt(int initReqCnt);
    int getInitReqCnt();
    /*
     * check if precapture trigger, disable sensor switch since then
     */
    void checkPrecaptureTrigger(const IMetadata* metadata, uint32_t requestNo);
    /*
     * reset precapture to false if capture is done
     */
    void resetPrecaptureTrigger();
    /*
     * check if touch af effects sensor switch
     */
    void checkAfDoneBeforeSwitch(const IMetadata* metadata, uint32_t requestNo);
    /*
     * notify af searching state and update mTouchAfReqList
     */
    void updateAfStateDone(uint32_t requestNo, uint32_t state);
public:
    /*
     * Return sensor state for specific id.
     * E_ONLINE: sensor already config p1node.
     * E_OFFLINE: sensor does not connect with p1node.
     * E_INVALID: not exist id.
     */
    SensorStateManager::SensorStateType getSensorState(uint32_t sensorId) const;
    /*
     * this value only effect only sensor state is ONLINE.
     * If query this function in OFFLINE state, it will crash.
     */
    sensorcontrol::SensorStatus getSensorOnlineState(uint32_t sensorId) const;
    void                        setSensorOnlineState(uint32_t const& id, sensorcontrol::SensorStatus state);
    void setSensorState(uint32_t const& id, SensorStateType type);

private:
    void setSwitchGoingFlag(bool flag);
    bool decideSensorOnOffState(
                sensorcontrol::SensorControlParamOut const& out,
                ParsedSensorStateResult &parsedSensorStateResult,
                std::unordered_map<uint32_t, sensorcontrol::SensorStatus>
                                        &vResidueOnlineStateList
            );
    bool updateSensorOnOffState(
                ParsedSensorStateResult const& parsedSensorStateResult
            );
    bool updateOnlineSensorResult(
                ParsedSensorStateResult &parsedSensorStateResult,
                std::unordered_map<uint32_t, sensorcontrol::SensorStatus>
                                        const& vResidueOnlineStateList
            );
    bool getQualityFlag(
                ParsedSensorStateResult const&result,
                Input const& inputParams,
                MUINT32 &qualityFlag);
    bool handleStreamingListForSwitchingFlow(
                ParsedSensorStateResult &parsedSensorStateResult
            );
    /*
     * reset idle count to all sensors.
     */
    bool resetIdleCountToAllSensors();
    /*
     * dump
     */
    bool dump(ParsedSensorStateResult &parsedSensorStateResult);
private:
    bool mbConfigDone = true;
    bool mbAfDone = true;
    bool mbNextCapture = true;
    //
    bool mbSensorSwitchGoing = false;
    // If there is uncomplete capture request It will set false
    bool mbSensorOKforSwitch = true;
    int32_t  mAppInitReqCnt = 0;
    int32_t  mInitReqCnt = 0;
    std::mutex mLock;
    // for request with af trigger
    std::vector<uint32_t> mTouchAfReqList;
    // for af trigger with cancel
    bool mbAfCancelTrigger = false;
    // precap is triggered or not
    bool mbPrecapTrigger = false;
    //
    uint32_t mLogLevel = 0;
    uint32_t mMaxOnlineSensorCount = 0;
    std::unordered_map<uint32_t, SensorState> mvSensorStateList;
    // for check switch time
    std::chrono::time_point<std::chrono::system_clock> pre_time;
    // store switching camera id
    std::vector<uint32_t> mvSwitchingIdList;
    // cache data(last updated)
    ParsedSensorStateResult mLastUpdateData;
    // in first request
    bool mbInit = true;
    //
    std::unique_ptr<
        SensorGroupManager,
        std::function<void(SensorGroupManager*)> > mpSensorGroupManager;
    //
    std::unique_ptr<
        ISPQualityManager,
        std::function<void(ISPQualityManager*)> > mpIspQualityManager;
};


/******************************************************************************
 *
 ******************************************************************************/
class SensorStatusDashboard {
  public:
    SensorStatusDashboard() = delete;
    SensorStatusDashboard(
        const std::vector<int32_t> vSensorId,
        uint32_t loglevel);

    ~SensorStatusDashboard();

    // This structure defines streaming sensor callback info from P1 result
    struct SensorRecord {
      uint32_t sensorId;
      int32_t frameNumber;
      bool isMaster = false;
    };

  public:
    // record decision streaming history
    bool updateDecision(uint32_t requestNo,
                        const std::vector<uint32_t>& streamingIds);

    // callback and record streaming sensors
    bool recordCallback(SensorStatusDashboard::SensorRecord const& record);

    // get current streaming sensors
    bool getStreamingIds(std::vector<int32_t>* pvSensorIds);

  private:
    //
    uint32_t mCurrentMasterSensor = -1;
    std::set<uint32_t> mvLatestStreamingSensor;

    // decision: <requestNo, streamingIds>
    std::unordered_map<int32_t, std::set<uint32_t>> mStreamingDecisionMap;

    // callback: <requestNo, streamingIds>
    std::unordered_map<int32_t, std::set<uint32_t>> mStreamingCallbackMap;

    //
    const std::vector<int32_t> mvSensorId;
    std::mutex mMutex;
    int32_t mLogLevel = 0;
};


/******************************************************************************
 *
 ******************************************************************************/
class RequestSensorControlPolicy_Multicam : public IRequestSensorControlPolicy
{
public:
    auto    evaluateConfiguration(
                        Configuration_SensorControl_Params params
                    ) -> int override;
    auto    evaluateRequest(
                        requestsensorcontrol::RequestOutputParams& out,
                        requestsensorcontrol::RequestInputParams const& in
                    ) -> int override;
    auto    sendPolicyDataCallback(
                        MUINTPTR arg1,
                        MUINTPTR arg2,
                        MUINTPTR arg3
                    ) -> bool override;

public:
    // RequestSensorControlPolicy Interfaces.
    RequestSensorControlPolicy_Multicam() = delete;
    RequestSensorControlPolicy_Multicam(CreationParams const& params);
    virtual ~RequestSensorControlPolicy_Multicam();

private:
    virtual auto    getPolicyData(
                                sensorcontrol::SensorControlParamIn &sensorControlIn,
                                requestsensorcontrol::RequestInputParams const& in) -> bool;
    virtual auto    get3AData(
                                int32_t sensorId,
                                std::shared_ptr<sensorcontrol::SensorControlInfo> &info) -> bool;
    virtual auto    getSensorData(
                                int32_t sensorId,
                                std::shared_ptr<sensorcontrol::SensorControlInfo> &info) -> bool;
    virtual auto    getDecisionResult(
                                sensorcontrol::SensorControlParamOut &out,
                                sensorcontrol::SensorControlParamIn &in) -> bool;

    virtual auto    updateP2List(
                                requestsensorcontrol::RequestInputParams const& in,
                                requestsensorcontrol::RequestOutputParams & out,
                                ParsedSensorStateResult const& result
                                ) -> bool;
    virtual auto    dumpSensorControlParam(
                                sensorcontrol::SensorControlParamOut &out,
                                sensorcontrol::SensorControlParamIn &in,
                                requestsensorcontrol::RequestInputParams const& req_in) -> bool;
    virtual auto    DetermineP1LaggingConfig(
                                Configuration_SensorControl_Params params,
                                SensorMap<bool> &vNeedLaggingConfigOut,
                                int32_t &masterId) -> bool;
    /*
     * according SensorControlParamOut to update sensor status.
     */
    virtual auto    handleSeamlessSwitch(
                                requestsensorcontrol::RequestInputParams const& in,
                                sensorcontrol::SensorControlParamOut &out,
                                bool& isSeamlessSwitching,
                                uint32_t& seamlessTargetSensorMode,
                                bool& isLosslessZoom,
                                uint32_t& losslessSensorMode
                                ) -> bool;

    virtual auto    convertRegions2SensorModeForSensorId(
                                MUINT32 sensorId,
                                MUINT32 targetMode,
                                sensorcontrol::SensorControlParamOut &out
                                ) -> bool;
private:
    CreationParams mPolicyParams;
    sensorcontrol::FeatureMode mFeatureMode = sensorcontrol::FeatureMode::E_None;
    //
    std::shared_ptr<sensorcontrol::ISensorControl> mpSensorControl = nullptr;
    std::shared_ptr<Hal3AObject> mpHal3AObj = nullptr;
    std::shared_ptr<DeviceInfoHelper> mpDeviceInfoHelper = nullptr;
    //
    std::unique_ptr<
        SensorStateManager,
        std::function<void(SensorStateManager*)> > mSensorStateManager;
    //
    std::unique_ptr<
        SensorControllerBehaviorManager,
        std::function<void(SensorControllerBehaviorManager*)> > mBehaviorManager;
    // record current currently effective sensor status
    std::unique_ptr<
        SensorStatusDashboard,
        std::function<void(SensorStatusDashboard*)>> mSensorStatusDashboard;
    //
    std::mutex mCapControlLock;
    uint32_t mCurrentActiveMasterId = -1;
    uint32_t mCurrentActiveFrameNo = -1;
    std::vector<uint32_t> mCaptureReqList;
    //
    int32_t mLogLevel = -1;
    //
    //bool mbNeedQueryAAOState = true;
    //bool mbNeedQeuryFrameSyncState = true;
    // When this flag is raised, it has to set specific metadata tag to notify 3A.
    bool mbNeedNotify3ASyncDone = false;
    // aao moniter list
    // when sensor switch is occure, it has to check both active sensor aao ready.
    std::vector<uint32_t> mvAAOMoniterList;
    // store preview streaming sensor id by request id.
    std::mutex mReqSensorStreamingMapLock;
    std::unordered_map<uint32_t, std::vector<uint32_t> > mvReqSensorStreamingMap;
    // capture request number list
    bool isCaptureing = false;
    int32_t capureCount = 1;
    bool isUseCaptureCountControl = false;
    //
    int mMaxStreamingSize = 0;
    bool mIsSeamlessModeAdopted = false;

private:  /// Seamless Switch
    SeamlessSwitchFeatureSetting mSeamlessSwitchConfiguration = {};
    uint32_t mMain1CurrentSensorMode = 0;
    float mZoom2TeleRatio = 0.0;
};

/******************************************************************************
 *
 ******************************************************************************/
class ISensorControllerBehavior;
class SensorControlBehavior;
class Sync3ABehavior;
class IspBehavior;
class FrameSyncBehavior;

class SensorControllerBehaviorManager
{
public:
    enum BEHAVIOR
    {
        Sync3A,
        FrameSync,
        SensorCrop,
        Streaming,
    };
public:
    SensorControllerBehaviorManager(
                int32_t logLevel);
    ~SensorControllerBehaviorManager();
public:
    bool update(
                requestsensorcontrol::RequestOutputParams& out,
                BehaviorUserData const& userData);
private:
    std::unordered_map<BEHAVIOR, std::shared_ptr<ISensorControllerBehavior> >
                    mvBehaviorList;
    //
    int32_t mLogLevel = -1;
};
/******************************************************************************
 *
 ******************************************************************************/
class ISensorControllerBehavior
{
public:
    ISensorControllerBehavior() = default;
    virtual ~ISensorControllerBehavior() = default;
public:
    virtual bool update(
                        RequestOutputParams& out,
                        BehaviorUserData const& userData) = 0;
};
/******************************************************************************
 *
 ******************************************************************************/
class SensorControllerBehaviorBase : public ISensorControllerBehavior
{
public:
    SensorControllerBehaviorBase(int32_t logLevel)
        : mLogLevel(logLevel)
    {
    }
    virtual ~SensorControllerBehaviorBase(){}
public:
    bool update(
        RequestOutputParams& out __attribute__((unused)),
        BehaviorUserData const& userData __attribute__((unused))) override
    {
        return false;
    }
protected:
    int32_t mLogLevel = -1;
};
/******************************************************************************
 *
 ******************************************************************************/
class Sync3ABehavior : public SensorControllerBehaviorBase
{
public:
    Sync3ABehavior(int32_t logLevel)
    : SensorControllerBehaviorBase(logLevel){}
    virtual ~Sync3ABehavior();
public:
    bool update(
                RequestOutputParams& out,
                BehaviorUserData const& userData __attribute__((unused))) override;

private:
    std::unordered_map<int32_t, bool> mvIsSensorAf;
};
/******************************************************************************
 *
 ******************************************************************************/
class FrameSyncBehavior : public SensorControllerBehaviorBase
{
public:
    FrameSyncBehavior(int32_t logLevel)
    : SensorControllerBehaviorBase(logLevel){}
    virtual ~FrameSyncBehavior();
public:
    bool update(
                RequestOutputParams& out,
                BehaviorUserData const& userData) override;
};
/******************************************************************************
 *
 ******************************************************************************/
class DomainConvert;
class SensorCropBehavior : public SensorControllerBehaviorBase
{
public:
    SensorCropBehavior(int32_t logLevel)
    : SensorControllerBehaviorBase(logLevel){}
    virtual ~SensorCropBehavior();
public:
    bool update(
                RequestOutputParams& out,
                BehaviorUserData const& userData) override;
private:
    // update sensor crop to metadata
    bool updateSensorCrop(
                RequestOutputParams& out,
                BehaviorUserData const& userData);
    bool getSensorCropRect(
                int32_t sensorId,
                std::unordered_map<uint32_t, MRect> const& vCropRegion,
                MRect &cropRect);
    bool get3aRegionRect(
                int32_t sensorId,
                std::unordered_map<uint32_t, std::vector<MINT32>> const& v3aRegion,
                std::vector<MINT32> &cropRect);

private:
    bool needBuildTransformMatrix = true;
    //
    std::unique_ptr<
        DomainConvert,
        std::function<void(DomainConvert*)> > mDomainConvert;
};
/******************************************************************************
 * convert active domain to tg domain
 ******************************************************************************/
class DomainConvert
{
public:
    DomainConvert(
            std::vector<int32_t> &sensorIdList,
            SensorMap<uint32_t> &sensorModeList);
    ~DomainConvert();
public:
    auto convertToTgDomain(
                        int32_t sensorId,
                        MRect &srcRect,
                        MRect &dstRect) -> bool;
        auto convertToTgDomain_simpletrans(
                       int32_t sensorId,
                       MRect &srcRect,
                       MRect &dstRect) -> bool;

private:
    auto buildTransformMatrix(
                        std::vector<int32_t> &sensorIdList,
                        SensorMap<uint32_t> &sensorModeList) -> bool;
private:
    std::unordered_map<int32_t, NSCamHW::HwMatrix>
                    mvTransformMatrix;
    std::unordered_map<int32_t, NSCam::v3::simpleTransform>
                    mvtransActive2Sensor;
};
/******************************************************************************
 *
 ******************************************************************************/
class StreamingBehavior : public SensorControllerBehaviorBase
{
public:
    StreamingBehavior(int32_t logLevel)
    : SensorControllerBehaviorBase(logLevel){}
    virtual ~StreamingBehavior();
public:
    bool update(
                RequestOutputParams& out,
                BehaviorUserData const& userData __attribute__((unused))) override;

private:
    float preZoomRatio = 1.0f;
    int count = 0;
};
};  //namespace requestsensorcontrol
};  //namespace policy
};  //namespace pipeline
};  //namespace v3
};  //namespace NSCam
#endif  //_MTK_HARDWARE_MTKCAM_PIPELINE_POLICY_REQUESTSENSORCONTROLPOLICY_MULTICAM_H_

