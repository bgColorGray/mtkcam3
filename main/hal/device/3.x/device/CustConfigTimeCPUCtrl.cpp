/* Copyright Statement:
 *
 *camera default CPU policy
 */

#include "CustConfigTimeCPUCtrl.h"
CustConfigTimeCPUCtrl::CustConfigTimeCPUCtrl() {
    loadPerfAPI(perfLockAcq, perfLockRel);
}
CustConfigTimeCPUCtrl:: ~CustConfigTimeCPUCtrl(){
    LOG_INF("~CustConfigTimeCPUCtrl");
    resetConfigTimeCPUCtrl();
}

    /* set camera default cpu policy start*/
void CustConfigTimeCPUCtrl:: loadPerfAPI(perfLockAcqFunc& lockAcq, perfLockRelFunc& lockRel) {
     static void* libHandle = NULL;  static void* funcA = NULL;
     static void* funcB = NULL;
     const char* perfLib = "libmtkperf_client_vendor.so";
     if (libHandle == NULL) {
         libHandle = dlopen(perfLib, RTLD_NOW);
         if (libHandle == NULL) {
             LOG_INF("dlopen fail: %s\n", dlerror());
             return;
         }
         LOG_INF("load lib");
     }
     if (funcA == NULL) {
         funcA = dlsym(libHandle, "perf_lock_acq");
     }
     if (funcA == NULL) {
         LOG_INF("perfLockAcq error: %s\n", dlerror());
         dlclose(libHandle);
         libHandle = NULL;
         return;
     }
     if (funcB == NULL) {
         funcB = dlsym(libHandle, "perf_lock_rel");
         if (funcB == NULL) {
             LOG_INF("perfLockRel error: %s\n", dlerror());
             dlclose(libHandle);
             libHandle = NULL;
             funcB = NULL;
             return;
         }
         LOG_INF("load funcB");
     }
    lockAcq = reinterpret_cast<perfLockAcqFunc>(funcA);
    lockRel = reinterpret_cast<perfLockRelFunc>(funcB);
}
void CustConfigTimeCPUCtrl:: resetConfigTimeCPUCtrl(){
     if (perfLockRel && mConfigTimeCPUCtrlHandle != 0) {
         perfLockRel(mConfigTimeCPUCtrlHandle);
         LOG_INF("Reset ConfigTimeCPUCtrl");
         mConfigTimeCPUCtrlHandle = 0;
     }
}
void CustConfigTimeCPUCtrl:: setConfigTimeCPUCtrl(int fps, int maxWidth) {
     if (!perfLockAcq) {
         return ;
     }
     resetConfigTimeCPUCtrl();
     LOG_INF("maxWidth%d fps:%d", maxWidth, fps);
     bool isLargeThan1080P = (maxWidth > 1920);
     LOG_INF("isLargeThan1080P:%d fps:%d", isLargeThan1080P, fps);
     std::unordered_map<std::string, int>
        custPolicy;
     for (auto& key : __availableKeys) {
          custPolicy.emplace(key, -1);
     }
        //if(fps <= 30) {
            //custPolicy["PERF_RES_FPS_FBT_RESCUE_C"] = 20;
            //custPolicy["PERF_RES_FPS_FBT_RESCUE_F"] = 0;
            //custPolicy["PERF_RES_FPS_FBT_RESCUE_F_OPP"] = 0;
            //custPolicy["PERF_RES_FPS_FBT_CEILING_ENABLE"] = 1;
            //custPolicy["PERF_RES_CFP_ENABLE"] = 0;
            //custPolicy["PERF_RES_FPS_FSTB_SOFT_FPS_LOWER"] = ::property_get_int32("vendor.camera.fpsgo.fps", fps);
            //custPolicy["PERF_RES_FPS_FSTB_SOFT_FPS_UPPER"] = ::property_get_int32("vendor.camera.fpsgo.fps", fps);
            //custPolicy["PERF_RES_FPS_FBT_GCC_HWUI_HINT"] = 0;
            //custPolicy["PERF_RES_FPS_FPSGO_ADJ_LOADING_HWUI_HINT"] = 0;
            //custPolicy["PERF_RES_FPS_FBT_GCC_DOWN_SEC_PCT"] = 10;
            //custPolicy["PERF_RES_FPS_FBT_BHR_OPP"] = 31;
            //custPolicy["PERF_RES_FPS_FPSGO_EMA_DIVIDEND"] = 2;
            // cpu floor
            //custPolicy["PERF_RES_CPUFREQ_MIN_CLUSTER_0"]=1800000;
            //custPolicy["PERF_RES_CPUFREQ_MIN_CLUSTER_1"]=2000000;
            // CPU Isolation
            //custPolicy["PERF_RES_SCHED_CORE_CTL_POLICY_ENABLE"] = 2;
            //custPolicy["PERF_RES_CPUCORE_MAX_CLUSTER_0"] = 4;
            //custPolicy["PERF_RES_CPUCORE_MIN_CLUSTER_0"] = 4;
            //custPolicy["PERF_RES_CPUCORE_MAX_CLUSTER_1"] = 3;
            //custPolicy["PERF_RES_CPUCORE_MIN_CLUSTER_1"] = 3;
            //custPolicy["PERF_RES_CPUCORE_MAX_CLUSTER_2"] = 1;
            //custPolicy["PERF_RES_CPUCORE_MIN_CLUSTER_2"] = 0;
            //}
        //else if (fps<=60 && !isLargeThan1080P) {
            //custPolicy["PERF_RES_FPS_FBT_RESCUE_C"] = 20;
            //}
        //else if (fps<=60 && isLargeThan1080P) {
            //custPolicy["PERF_RES_FPS_FBT_RESCUE_C"] = 20;
            //}
        /* just for MT6877-T0 60fps enable prefer_idle */
     if (fps > 30) {
         custPolicy["PERF_RES_SCHED_PREFER_IDLE_TA"] = 1;
         custPolicy["PERF_RES_SCHED_PREFER_IDLE_FG"] = 1;
         custPolicy["PERF_RES_FPS_FPSGO_IDLEPREFER"] = 1;
     }
     size_t custSize = 0;
     for (auto& o : custPolicy) {
          if (o.second >= 0) {
              custSize += 2; // key + value
          }
     }
     std::vector<int> custCmd;
     custCmd.reserve(custSize);
     std::ostringstream oss;
     oss << "customizeCPUCtrl params:" << std::endl;
     for (auto& key : __availableKeys) {
          auto it = custPolicy.find(key);
          if (it != custPolicy.end() && it->second >= 0) {
              auto commandIt = StringToPowerEnumMap.find(key);
              if (commandIt != StringToPowerEnumMap.end()) {
                  custCmd.push_back(commandIt->second);
                  custCmd.push_back(it->second);
                  oss << "  " << key << ": " << it->second << std::endl;
               }
          }
     }
     LOG_INF("%s", oss.str().c_str());
     if(perfLockAcq != NULL){
        mConfigTimeCPUCtrlHandle = perfLockAcq(
        mConfigTimeCPUCtrlHandle, 0 /*timout, unit is ms*/,
        custCmd.data(), custCmd.size());
        LOG_INF("Run customzied FPS Go with param(handle: %d)",
        mConfigTimeCPUCtrlHandle);
     }
}