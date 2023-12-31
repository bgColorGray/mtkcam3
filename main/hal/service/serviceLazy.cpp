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

#define LOG_TAG "camerahalserver"

#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
//#include <../devicemgr/provider/2.4/CameraProviderImpl.h>
#include <dlfcn.h>
#include <signal.h>

#ifdef MTKCAM_BGSERVICE_SUPPORT
//#include <bgservice/BGService.h>
#include <vendor/mediatek/hardware/camera/bgservice/1.1/IBGService.h>
#endif

#include <vendor/mediatek/hardware/camera/atms/1.0/IATMs.h>

#include <cutils/compiler.h>

#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <android/hidl/manager/1.2/IClientCallback.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl/HidlSupport.h>

#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <hidl/HidlLazyUtils.h>


#include <binder/ProcessState.h>
#include <mtkcam/utils/std/ULog.h>
//
#include <mtkcam/custom/ExifFactory.h>

using namespace android;
using namespace android::hardware;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::LazyServiceRegistrar;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::camera::provider::V2_6::ICameraProvider;
using ::android::hidl::manager::V1_2::IClientCallback;
using ::android::hidl::manager::V1_2::IServiceManager;

#define MY_LOGI_IF(cond, ...)   do { if (            (cond) ) { ALOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)   do { if ( CC_UNLIKELY(cond) ) { ALOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)   do { if ( CC_UNLIKELY(cond) ) { ALOGE(__VA_ARGS__); } }while(0)


#if (PLATFORM_SDK_VERSION >= 21)
    //#pragma message("camerahal serviceLazy")
    #define CAM_PROVIDER_IMPL_PATH "hw/android.hardware.camera.provider@2.6-impl-mediatek.so"
    #define CAM_BGS_IMPL_PATH "hw/vendor.mediatek.hardware.camera.bgservice@1.1-impl.so"
#else
    #ifdef __LP64__
        //#pragma message("camerahal serviceLazy 64 bits")
        #define CAM_PROVIDER_IMPL_PATH "/vendor/lib64/hw/android.hardware.camera.provider@2.6-impl-mediatek.so"
        #define CAM_BGS_IMPL_PATH "/vendor/lib64/hw/vendor.mediatek.hardware.camera.bgservice@1.1-impl.so"
    #else
        //#pragma message("camerahal serviceLazy 32 bits")
        #define CAM_PROVIDER_IMPL_PATH "/vendor/lib/hw/android.hardware.camera.provider@2.6-impl-mediatek.so"
        #define CAM_BGS_IMPL_PATH "/vendor/lib/hw/vendor.mediatek.hardware.camera.bgservice@1.1-impl.so"
    #endif
#endif

template<typename INTERFACE>
class Cb : public IClientCallback {
    public:
        Cb(const char* serviceName) :
            mServiceName(serviceName){
        }

    private:
        Return<void> onClients(const sp<IBase>& base, bool clients) override {
            ALOGD("LazyHAL %s/%s onClients, has clients:%s", INTERFACE::descriptor, mServiceName, clients ? "true":"false");
            // we have no clients after having clients at least once
            if (!clients) {
                sp<IServiceManager> manager = defaultServiceManager1_2();
                bool ret = manager->tryUnregister(INTERFACE::descriptor, mServiceName, base);

                if(ret) {
                    //Terminate process camerahal
                    ALOGI("LazyHAL triggered Camera Lazy HAL Server suiciding...");
                    if(raise(SIGKILL))
                    {
                        ALOGE("raise signal 9 failed !");
                    }
                }
                else {
                    ALOGW("LazyHAL %s/%s tryUnregister fail, camerahalserver terminate abandoned", INTERFACE::descriptor, mServiceName);
                }
            }

            return Status::ok();
        }

    private:
        const char* mServiceName;
};


/*********************************************
 * symName : function name in .so
 * paramStr: param of symName
 * serviceName: name of lazy hidl/service
 *********************************************/
template<typename INTERFACE>
auto registerLazyService(const char* libPath, const char* symName, const char* paramStr, const char* serviceName)
{
    void* libPtr = ::dlopen(libPath, RTLD_NOW);
    if (!libPtr){
        char const *err_str = ::dlerror();
        ALOGE("dlopen: %s error=%s", libPath, (err_str ? err_str : "unknown"));
        return -1;
    }

    typedef INTERFACE* (*pfnEntry_T)(const char* name);
    pfnEntry_T pfunc = (pfnEntry_T) ::dlsym(libPtr, symName);
    if(!pfunc){
        char const *err_str = ::dlerror();
        ALOGE("dlsym: %s error=%s", symName, (err_str ? err_str : "unknown"));
        return -1;
    }

    INTERFACE* provider = pfunc(paramStr);
    auto err = LazyServiceRegistrar::getInstance().registerService(provider, serviceName);

    if(libPtr){
        ::dlclose(libPtr);
        libPtr = NULL;
    }

    return err;
}


template<typename INTERFACE>
auto registerServiceWithCb(const char* libPath, const char* symName, const char* pFuncName, const char* serviceName)
{
    void* libPtr = ::dlopen(libPath, RTLD_NOW);
    if (!libPtr){
        char const *err_str = ::dlerror();
        ALOGE("dlopen: %s error=%s", libPath, (err_str ? err_str : "unknown"));
        return -1;
    }

    typedef INTERFACE* (*pfnEntry_T)(const char* name);
    pfnEntry_T pfunc = (pfnEntry_T) ::dlsym(libPtr, symName);
    if(!pfunc){
        char const *err_str = ::dlerror();
        ALOGE("dlsym: %s error=%s", symName, (err_str ? err_str : "unknown"));
        if(libPtr){
           ::dlclose(libPtr);
           libPtr = NULL;
        }
        return -1;
    }

    //auto err = registerLazyPassthroughServiceImplementation<INTERFACE>(serviceName);

    INTERFACE* service = pfunc(pFuncName);
    if(!service){
        char const *err_str = ::dlerror();
        ALOGE("pfunc: %s error=%s", pFuncName, (err_str ? err_str : "unknown"));
        if(libPtr){
           ::dlclose(libPtr);
           libPtr = NULL;
        }
        return -1;
    }
    auto err = service->registerAsService(serviceName);
    MY_LOGE_IF(err!=OK, "LazyHAL Failed to register %s! err:%d(%s)", serviceName, err, ::strerror(-err));

    sp<IServiceManager> manager = defaultServiceManager1_2();
    auto ret = manager->registerClientCallback(INTERFACE::descriptor, serviceName, service, new Cb<INTERFACE>(serviceName));
    if (!ret.withDefault(false)) {
        // if it is okay, then it must be false success return
        ALOGE("LazyHAL IClientCallback failed to register: %s", ret.description().c_str());
    }

    if(libPtr){
        ::dlclose(libPtr);
        libPtr = NULL;
    }

    return err;
}

void signal_handler(int s) {
    ALOGI("Caught SIGPIPE\n");
}

int main()
{
    ALOGI("Camera Lazy HAL Server is starting...");
    NSCam::Utils::ULog::ULogInitializer __ulogInitializer;

    if((signal(SIGPIPE, signal_handler))==SIG_ERR){
        ALOGE("Signal call failure !");
    }

    // The camera HAL may communicate to other vendor components via
    // /dev/vndbinder
    android::ProcessState::initWithDriver("/dev/vndbinder");


    configureRpcThreadpool(16, true /*callerWillJoin*/);

    //  AOSP ICameraProvider HAL Interface
#if 0 //Not use DYNAMIC_SHUTTDOWN
    {
        auto err = registerLazyService<ICameraProvider>(CAM_PROVIDER_IMPL_PATH, "HIDL_FETCH_ICameraProvider", "internal/0", "internal/0");
        MY_LOGW_IF( err!=OK, "%s: register Lazy ICameraProvider err:%d(%s)", __FUNCTION__, err, ::strerror(-err));
    }
#else
    {
        using android::hardware::camera::provider::V2_6::ICameraProvider;
        auto err = registerServiceWithCb<ICameraProvider>(CAM_PROVIDER_IMPL_PATH, "HIDL_FETCH_ICameraProvider", "internal/0", "internal/0");
        MY_LOGW_IF( err!=OK, "%s: register ICameraProvider err:%d(%s)", __FUNCTION__, err, ::strerror(-err));
    }
#endif

    {
#ifdef MTKCAM_BGSERVICE_SUPPORT
        using ::vendor::mediatek::hardware::camera::bgservice::V1_1::IBGService;
        auto err = registerPassthroughServiceImplementation<IBGService>("internal/0" /*"internal" for binderized mode*/);
        MY_LOGW_IF( err!=OK, "%s: register IBGService err:%d(%s)", __FUNCTION__, err, ::strerror(-err));
#endif
    }

    {
        using ::vendor::mediatek::hardware::camera::atms::V1_0::IATMs;
        auto err = registerPassthroughServiceImplementation<IATMs>();
        MY_LOGW_IF( err!=OK, "%s: register IATMs err:%d(%s)", __FUNCTION__, err, ::strerror(-err));
    }

    //make debugexif instance while process starts
    {
        auto inst = MAKE_DebugExif();
        MY_LOGW_IF(!inst, "%s: bad getDebugExif()", __FUNCTION__);
    }

    joinRpcThreadpool();
    return 0;
}

