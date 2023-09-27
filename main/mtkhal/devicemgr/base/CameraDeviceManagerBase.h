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

#ifndef MAIN_MTKHAL_DEVICEMGR_BASE_CAMERADEVICEMANAGERBASE_H_
#define MAIN_MTKHAL_DEVICEMGR_BASE_CAMERADEVICEMANAGERBASE_H_
//
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProvider.h>
#include <mtkcam3/main/mtkhal/core/provider/2.x/IMtkcamProviderCallback.h>
#include <mtkcam3/main/mtkhal/devicemgr/ICameraDeviceManager.h>
#include <mtkcam/utils/LogicalCam/IHalLogicalDeviceList.h>
#include <mtkcam/utils/metastore/IMetadataProvider.h>

#include <semaphore.h>
#include <utils/KeyedVector.h>
#include <utils/Mutex.h>
#include <utils/RWLock.h>
#include <utils/String8.h>

#include <list>
#include <map>
#include <vector>
#include <memory>
#include <string>
//
/******************************************************************************
 *
 ******************************************************************************/
using mcam::IMtkcamProvider;
using mcam::IMtkcamProviderCallback;
using mcam::IMtkcamDevice;

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
class CameraDeviceManagerBase : public ICameraDeviceManager,
                                public IMtkcamProvider {
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Definitions.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  enum class ETorchModeStatus : uint32_t {
    NOT_AVAILABLE = 0u,
    AVAILABLE_OFF = 1u,
    AVAILABLE_ON = 2u,
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 public:  //// Physical Enumerating Device
  class PhysEnumDevice : public android::RefBase {
   public:  ////                        fields.
    ::android::sp<IMetadataProvider> mMetadataProvider;  // Metadata Provider
    std::string mSensorName;  // Sensor (driver) Name.
    int32_t mInstanceId;      // Device Instance Id (0, 1, 2, ...).
    int32_t mFacing;          // Device Facing Direction (MTK_LENS_FACING_FRONT,
                              // MTK_LENS_FACING_BACK).
    int32_t mWantedOrientation;  // Device Wanted Orientation.
    int32_t mSetupOrientation;   // Device Setup Orientation.
    int32_t mHasFlashUnit;       // Device Flash Unit
   public:                       ////                        operations.
    PhysEnumDevice();
  };

  //  PhysEnumDeviceMap_t : instance id -> physical device
  using PhysEnumDeviceMap_t =
      ::android::DefaultKeyedVector<int32_t, ::android::sp<PhysEnumDevice>>;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 public:  //// Virtual Enumerating Device
  class VirtEnumDevice : public android::RefBase {
   public:  ////                        fields.
    std::shared_ptr<IVirtualDevice> mVirtDevice;  // Virtual Device
    int32_t mInstanceId;        // Device Instance Id (0, 1, 2, ...).
    uint32_t mTorchModeStatus;  // ETorchModeStatus
    bool mTorchModeStatusChanging;

   public:  ////                        operations.
    VirtEnumDevice();
  };

  //  VirtEnumDeviceMap_t  : cameraDeviceId -> virtual device
  using VirtEnumDeviceMap_t =
      ::android::DefaultKeyedVector<int32_t, ::android::sp<VirtEnumDevice>>;

 public:  ////
  struct VirtEnumDeviceFactory {
    using FactoryT = IVirtualDevice* (*)(CreateVirtualCameraDeviceParams*);
    FactoryT mCreateVirtualCameraDevice = nullptr;
    void* mLibHandle = nullptr;
  };

  //  Device major version -> factory info
  using VirtEnumDeviceFactoryMap_t = std::map<uint32_t, VirtEnumDeviceFactory>;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 public:  ////
  class OpenDevice : public android::RefBase {
   public:  ////                        fields.
    ::android::sp<PhysEnumDevice> mPhysDevice;  //  Physical Device
    std::shared_ptr<IVirtualDevice> mVirtDevice;  //  Virtual Device
    uint32_t mMajorDeviceVersion;
    uint32_t mMinorDeviceVersion;
    ::std::string mOpenTimestamp;

   public:  ////                        operations.
    OpenDevice();
  };

  //  OpenInfoMap_t :  instance id -> open info
  using OpenDeviceMap_t =
      ::android::DefaultKeyedVector<int32_t, ::android::sp<OpenDevice>>;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 public:  ////
  class ActiveOperation {
   public:  ////
    enum OPERATION {
      IDLE,
      OPEN,
      CLOSE,
      SET_TORCH,
    };
    mutable android::Mutex mOpsLock;
    mutable android::RWLock mDataRWLock;
    uint32_t mOperation;  //  OPERATION
    ::std::string mTimestamp;
    std::shared_ptr<IVirtualDevice> mVirtDevice;  //  Virtual Device

   public:  ////        operations.
    ActiveOperation();
    auto lock(  //  lock & set
        const std::shared_ptr<IVirtualDevice>& pVirtualDevice,
        uint32_t operation) -> ::android::status_t;

    auto unlock(  //  reset & unlock
        const std::shared_ptr<IVirtualDevice>& pVirtualDevice,
        uint32_t operation) -> void;

    auto validate(const std::shared_ptr<IVirtualDevice>& pVirtualDevice,
                  uint32_t operation) const -> ::android::status_t;

    auto logIfOwnerExist(
        ::android::Vector<::android::String8>* pLogs = nullptr) const -> void;
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 public:  ////
  class ICommand : public android::RefBase {
   public:  ////
    virtual auto doExecute() -> ::android::status_t = 0;
    virtual auto undoExecute() -> ::android::status_t = 0;
  };

  using CommandList_t = std::list<::android::sp<ICommand>>;

  class SetTorchModeStatusCommand : public ICommand {
   public:
    virtual auto doExecute() -> ::android::status_t;
    virtual auto undoExecute() -> ::android::status_t;

    SetTorchModeStatusCommand(
        CameraDeviceManagerBase* pManager,
        const std::shared_ptr<IVirtualDevice>& pVirtualDevice,
        ETorchModeStatus newTorchModeStatus);

   protected:
    CameraDeviceManagerBase* mpManager;
    std::shared_ptr<IVirtualDevice> mpVirtualDevice;
    ETorchModeStatus mOldStatus;
    ETorchModeStatus mNewStatus;
  };

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  Implementations.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 protected:  ////                Data Members.
  /**
   *  stages: constructor.
   */
  const std::string mType;
  const std::string mCreationTimestamp;
  bool mSupportLegacyDevice = true;
  bool mSupportLatestDevice = false;
  std::vector<int>
      mOverriddenDeviceHalVersions;  // override the device hal major version
  VirtEnumDeviceFactoryMap_t mVirtEnumDeviceFactoryMap;

  /**
   *  stages: enumeration and normal operations.
   */
  mutable android::RWLock mDataRWLock;

  bool mIsSetTorchModeSupported;  // setup at the stage of enumeration operation
  PhysEnumDeviceMap_t
      mPhysEnumDeviceMap;  // setup at the stage of enumeration operation
  VirtEnumDeviceMap_t mVirtEnumDeviceMap;  // setup at the stage of
                                           // enumeration/normal operations
  ::android::DefaultKeyedVector<int32_t, std::string>
      mVirInstanceMap;  // setup at the stage of enumeration/normal operations
  OpenDeviceMap_t mOpenDeviceMap;     // setup at the stage of normal operation
  std::shared_ptr<IMtkcamProviderCallback>
      mCallback;  // setup at the stage of normal operation
  sem_t mSemPowerOn;
  ESecureModeStatus mSecureStatus;

  using HiddenDeviceMap_t =
      ::android::DefaultKeyedVector<int, bool>;  // open ID -> is hidden
  HiddenDeviceMap_t mHiddenCameraMap;
  bool mIsCameraHiddenEnabled = false;

  /**
   *  startOpenDevice/finishOpenDevice and startCloseDevice/finishCloseDevice
   *  1) must be called in pairs, respectively.
   *  2) must be called atomically before another one pair.
   *
   *  mActiveOperation.lock() is used to make sure the operations among
   *  each startXXXDevice and setTorchMode are mutually exclusive.
   *
   *  mActiveOperation.validate() is used in finishXXXDevice to check to see
   *  whether it is the owner to call startXXXDevice.
   *
   *  mActiveOperationCommandList is used to save the commands invoked in
   *  startXXXDevice, so that it could be used to undo or restore those
   *  commands in finishXXXDevice if needed. mActiveOperationCommandList is
   *  reset in finishXXXDevice.
   */
  ActiveOperation mActiveOperation;
  CommandList_t mActiveOperationCommandList;

 public:  ////    Instantiation.
  explicit CameraDeviceManagerBase(char const* type);
  virtual ~CameraDeviceManagerBase();
  virtual auto initialize() -> bool;

 protected:  ////    Operations - camera: enumeration
  virtual auto enumerateDevicesLocked() -> ::android::status_t;
  virtual auto onEnumerateDevicesLocked() -> ::android::status_t = 0;
  virtual auto addVirtualDevicesLocked(
      uint32_t const instanceId,
      uint32_t const virtualInstanceId,
      bool const isSetHidden,
      ::android::sp<IMetadataProvider> pMetadataProvider,
      std::map<uint32_t, ::android::sp<IMetadataProvider>> const&
          physicalMetadataProviders) -> void;

 protected:  ////    Operations - camera: open
  virtual auto onGetMaxNumOfMultiOpenCameras() const -> uint32_t = 0;

  virtual auto validateOpenLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice) const
      -> ::android::status_t;

  virtual auto onValidateOpenLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice) const
      -> ::android::status_t = 0;

  virtual auto attachOpenDeviceLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice)
      -> ::android::status_t;

  virtual auto detachOpenDeviceLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice)
      -> ::android::status_t;

  virtual auto onAttachOpenDeviceLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice) -> void = 0;

  virtual auto onDetachOpenDeviceLocked(
      const std::shared_ptr<IVirtualDevice>& pVirtualDevice) -> void = 0;

 protected:  ////    Operations - torch
  virtual auto isTorchModeSupportedLocked(const std::string& deviceName) const
      -> ::android::status_t;

  virtual auto isTorchModeControllableLocked(
      const std::string& deviceName) const -> ::android::status_t;

  virtual auto flushTorchModeStatusChangeCallback() -> ::android::status_t;

  virtual auto addTorchModeStatusChangeLocked(const std::string& deviceName,
                                              bool enable) -> void;

  virtual auto setTorchModeStatusWhenOpenCloseCameraLocked(
      const std::string& deviceName,
      uint32_t newTorchModeStatus,
      bool markAsChanging) -> ::android::status_t;

  virtual auto onEnableTorchLocked(const std::string& deviceName, bool enable)
      -> ::android::status_t = 0;

 protected:  ////    Operations - utils
  static auto convertFromLegacyDeviceVersion(uint32_t legacyVersion,
                                             uint32_t* major = nullptr,
                                             uint32_t* minor = nullptr) -> void;

  static auto queryMajorDeviceVersion(uint32_t legacyVersion) -> uint32_t;

  virtual auto getVirtualDeviceLocked(
      const std::string& deviceName,
      ::android::sp<VirtEnumDevice>* ppInfo,
      std::shared_ptr<IVirtualDevice>* ppVirtualDevice) const
      -> ::android::status_t;

  virtual auto logLocked(
      ::android::Vector<::android::String8>* pLogs = nullptr) const -> void;

  virtual auto getDeviceName(const int& deviceId, std::string* rDeviceName)
      -> int;

  virtual auto getDeviceIdWithPhysicalId(const int physicalId, int& deviceId)
      -> void;

  virtual auto waitForPreviousPowerOnDone() -> void;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  ICamDeviceManager Interfaces.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 public:  ////
  virtual auto debug(std::shared_ptr<IPrinter> printer,
                     const std::vector<std::string>& options) -> void;

  virtual auto setCallback(
      const std::shared_ptr<IMtkcamProviderCallback>& callback)
      -> ::android::status_t;

  virtual auto  getCameraIdList(std::vector<int32_t>& rCameraDeviceIds) const
                  -> int;

  virtual auto  getVendorTags(
                  std::vector<mcam::VendorTagSection>& sections) const
                  -> void;

//  virtual auto getDeviceInfoList(
//      std::vector<mcam::DeviceInfo>& rDeviceInfos) -> int;

  virtual auto getDeviceInterface(const int32_t cameraDeviceId,
                                std::shared_ptr<IMtkcamDevice>& rpDevice) const
      -> int;

  virtual auto getType() const -> const std::string&;

  virtual auto isSetTorchModeSupported() const -> bool;

  virtual auto setTorchMode(const std::string& deviceName, uint32_t mode)
      -> int;

  virtual auto getConcurrentStreamingCameraIds(
          std::vector<std::vector<int32_t>>& cameraIds) const
      -> int;

  virtual auto isConcurrentStreamCombinationSupported(
          std::vector<mcam::CameraIdAndStreamCombination>& configs,
          bool& queryStatus) const
      -> int;

  virtual auto startOpenDevice(const std::string& deviceName)
      -> ::android::status_t;

  virtual auto notifyDeviceStateChange(mcam::DeviceState newState)
      -> int;

  virtual auto finishOpenDevice(const std::string& deviceName,
                                bool cancel /*cancel open if true*/)
      -> ::android::status_t;

  virtual auto startCloseDevice(const std::string& deviceName)
      -> ::android::status_t;

  virtual auto finishCloseDevice(const std::string& deviceName)
      -> ::android::status_t;
  virtual auto getOpenedCameraNum() -> uint32_t;
  virtual auto updatePowerOnDone() -> void;
  virtual auto setSecureMode(uint32_t secureMode)
      -> ::android::status_t;

  virtual auto isHiddenCamera(int sensorId) const -> bool;
};
};      // namespace NSCam
#endif  // MAIN_MTKHAL_DEVICEMGR_BASE_CAMERADEVICEMANAGERBASE_H_
