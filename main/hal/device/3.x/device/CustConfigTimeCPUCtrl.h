/* Copyright Statement:
 *
 *camera default CPU policy
 */
#ifndef _MTK_CUSTCONFIGTIMECPUCTRL_H
#define _MTK_CUSTCONFIGTIMECPUCTRL_H
#include <cutils/properties.h>
#include <mtkcam/utils/std/ULog.h>
// will include <log/log.h> if LOG_TAG was defined
#include <dlfcn.h>
#include <fcntl.h>
#include <mtkperf_resource.h>
#include<unordered_set>
#include <system/camera_metadata.h>
#include<unordered_map>
#include<sstream>
#include <set>
using namespace android;
using perfLockAcqFunc = int (*)(int, int, int[], int);
using perfLockRelFunc = int (*)(int);
#define LOG_INF(fmt, arg...) ALOGI("[%s] " fmt, __FUNCTION__, ##arg)

class CustConfigTimeCPUCtrl{
    public :
    CustConfigTimeCPUCtrl();
    virtual ~CustConfigTimeCPUCtrl();
    void loadPerfAPI(perfLockAcqFunc& lockAcq,perfLockRelFunc& lockRel);
    void setConfigTimeCPUCtrl(int fps, int maxWidth);
    void resetConfigTimeCPUCtrl();
    private:
        int mConfigTimeCPUCtrlHandle = 0;
        perfLockAcqFunc perfLockAcq = NULL;
        perfLockRelFunc perfLockRel = NULL;
    const std::unordered_set<std::string> __availableKeys = {
        // FPS Go
        "PERF_RES_FPS_FBT_BHR",
        "PERF_RES_FPS_FBT_BHR_OPP",
        "PERF_RES_FPS_FBT_BOOST_TA",
        "PERF_RES_FPS_FBT_CEILING_ENABLE",
        "PERF_RES_FPS_FBT_FLOOR_BOUND",
        "PERF_RES_FPS_FBT_GCC_DEQ_BOUND_QUOTA",
        "PERF_RES_FPS_FBT_GCC_DEQ_BOUND_THRS",
        "PERF_RES_FPS_FBT_GCC_DOWN_QUOTA_PCT",
        "PERF_RES_FPS_FBT_GCC_DOWN_SEC_PCT",
        "PERF_RES_FPS_FBT_GCC_DOWN_STEP",
        "PERF_RES_FPS_FBT_GCC_ENQ_BOUND_QUOTA",
        "PERF_RES_FPS_FBT_GCC_ENQ_BOUND_THRS",
        "PERF_RES_FPS_FBT_GCC_FPS_MARGIN",
        "PERF_RES_FPS_FBT_GCC_POSITIVE_CLAMP",
        "PERF_RES_FPS_FBT_GCC_UP_QUOTA_PCT",
        "PERF_RES_FPS_FBT_GCC_UP_SEC_PCT",
        "PERF_RES_FPS_FBT_GCC_UP_STEP",
        "PERF_RES_FPS_FBT_GCC_UPPER_CLAMP",
        "PERF_RES_FPS_FBT_KMIN",
        "PERF_RES_FPS_FBT_LIMIT_CFREQ",
        "PERF_RES_FPS_FBT_LIMIT_CFREQ_M",
        "PERF_RES_FPS_FBT_LIMIT_RFREQ",
        "PERF_RES_FPS_FBT_LIMIT_RFREQ_M",
        "PERF_RES_FPS_FBT_MIN_RESCUE_PERCENT",
        "PERF_RES_FPS_FBT_QR_ENABLE",
        "PERF_RES_FPS_FPSGO_ADJ_LOADING_HWUI_HINT",
        "PERF_RES_FPS_FBT_GCC_HWUI_HINT",
        "PERF_RES_FPS_FBT_QR_HWUI_HINT",
        "PERF_RES_FPS_FBT_QR_T2WNT_X",
        "PERF_RES_FPS_FBT_QR_T2WNT_Y_N",
        "PERF_RES_FPS_FBT_QR_T2WNT_Y_P",
        "PERF_RES_FPS_FBT_RESCUE_C",
        "PERF_RES_FPS_FBT_RESCUE_F",
        "PERF_RES_FPS_FBT_RESCUE_F_OPP",
        "PERF_RES_FPS_FBT_RESCUE_PERCENT",
        "PERF_RES_FPS_FBT_RESCUE_SECOND_ENABLE",
        "PERF_RES_FPS_FBT_RESCUE_SECOND_GROUP",
        "PERF_RES_FPS_FBT_RESCUE_SECOND_TIME",
        "PERF_RES_FPS_FBT_SHORT_RESCUE_NS",
        "PERF_RES_FPS_FBT_ULTRA_RESCUE",
        "PERF_RES_FPS_FPSGO_ADJ_CNT",
        "PERF_RES_FPS_FPSGO_ADJ_DEBNC_CNT",
        "PERF_RES_FPS_FPSGO_ADJ_LOADING",
        "PERF_RES_FPS_FPSGO_ADJ_LOADING_TIMEDIFF",
        "PERF_RES_FPS_FPSGO_BOOST_AFFINITY",
        "PERF_RES_FPS_FPSGO_CM_BIG_CAP",
        "PERF_RES_FPS_FPSGO_CM_TDIFF",
        "PERF_RES_FPS_FPSGO_DEP_FRAMES",
        "PERF_RES_FPS_FPSGO_EMA_DIVIDEND",
        "PERF_RES_FPS_FPSGO_ENABLE",
        "PERF_RES_FPS_FPSGO_FORCE_ONOFF",
        "PERF_RES_FPS_FPSGO_GPU_BLOCK_BOOST",
        "PERF_RES_FPS_FPSGO_IDLEPREFER",
        "PERF_RES_FPS_FPSGO_LLF_LIGHT_LOADING_POLICY",
        "PERF_RES_FPS_FPSGO_LLF_POLICY",
        "PERF_RES_FPS_FPSGO_LLF_TH",
        "PERF_RES_FPS_FPSGO_MARGIN_MODE",
        "PERF_RES_FPS_FPSGO_MARGIN_MODE_DBNC_A",
        "PERF_RES_FPS_FPSGO_MARGIN_MODE_DBNC_B",
        "PERF_RES_FPS_FPSGO_MARGIN_MODE_GPU",
        "PERF_RES_FPS_FPSGO_SP_CK_PERIOD",
        "PERF_RES_FPS_FPSGO_SP_NAME_ID",
        "PERF_RES_FPS_FPSGO_SP_SUB",
        "PERF_RES_FPS_FPSGO_SPID",
        "PERF_RES_FPS_FPSGO_STDDEV_MULTI",
        "PERF_RES_FPS_FPSGO_SUB_QUE_DEQUE_PERIOD",
        "PERF_RES_FPS_FPSGO_THRM_ENABLE",
        "PERF_RES_FPS_FPSGO_THRM_LIMIT_CPU",
        "PERF_RES_FPS_FPSGO_THRM_SUB_CPU",
        "PERF_RES_FPS_FPSGO_THRM_TEMP_TH",
        "PERF_RES_FPS_FSTB_JUMP_CHECK_NUM",
        "PERF_RES_FPS_FSTB_JUMP_CHECK_Q_PCT",
        "PERF_RES_FPS_FSTB_SOFT_FPS_LOWER",
        "PERF_RES_FPS_FSTB_SOFT_FPS_UPPER",
        /* MAJOR = 1 CPU freq*/
        "PERF_RES_CPUFREQ_MIN_CLUSTER_0",
        "PERF_RES_CPUFREQ_MIN_CLUSTER_1",
        "PERF_RES_CPUFREQ_MIN_CLUSTER_2",
        "PERF_RES_CPUFREQ_MAX_CLUSTER_0",
        "PERF_RES_CPUFREQ_MAX_CLUSTER_1",
        "PERF_RES_CPUFREQ_MAX_CLUSTER_2",
        "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0",
        "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1",
        "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2",
        "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0",
        "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1",
        "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2",
        "PERF_RES_CPUFREQ_CCI_FREQ",
        "PERF_RES_CPUFREQ_PERF_MODE",
        /* MAJOR = 2 CPU core*/
        "PERF_RES_CPUCORE_MIN_CLUSTER_0",
        "PERF_RES_CPUCORE_MIN_CLUSTER_1",
        "PERF_RES_CPUCORE_MIN_CLUSTER_2",
        "PERF_RES_CPUCORE_MAX_CLUSTER_0",
        "PERF_RES_CPUCORE_MAX_CLUSTER_1",
        "PERF_RES_CPUCORE_MAX_CLUSTER_2",
        "PERF_RES_CPUCORE_PERF_MODE",
        "PERF_RES_CPUCORE_FORCE_PAUSE_CPU",
        /* MAJOR = 3 GPU*/
        "PERF_RES_GPU_FREQ_MIN",
        "PERF_RES_GPU_FREQ_MAX",
        "PERF_RES_GPU_FREQ_LOW_LATENCY",
        "PERF_RES_GPU_GED_MARGIN_MODE",
        "PERF_RES_GPU_GED_TIMER_BASE_DVFS_MARGIN",
        "PERF_RES_GPU_GED_LOADING_BASE_DVFS_STEP",
        "PERF_RES_GPU_GED_CWAITG",
        "PERF_RES_GPU_GED_GX_BOOST",
        "PERF_RES_GPU_GED_DVFS_LOADING_MODE",
        "PERF_RES_GPU_POWER_POLICY",
        "PERF_RES_GPU_POWER_ONOFF_INTERVAL",
        /* MAJOR = 4 DRAM (VCORE, CM MGR, EMI)*/
        "PERF_RES_DRAM_OPP_MIN",
        "PERF_RES_DRAM_OPP_MIN_LP5",
        "PERF_RES_DRAM_OPP_MIN_LP5_HFR",
        "PERF_RES_DRAM_VCORE_MIN",
        "PERF_RES_DRAM_VCORE_MIN_LP3",
        "PERF_RES_DRAM_VCORE_BW_ENABLE",
        "PERF_RES_DRAM_VCORE_BW_THRES",
        "PERF_RES_DRAM_VCORE_BW_THRESH_LP3",
        "PERF_RES_DRAM_VCORE_POLICY",
        "PERF_RES_DRAM_CM_MGR",
        "PERF_RES_DRAM_CM_MGR_CAM_ENABLE",
        "PERF_RES_DRAM_CM_RATIO_UP_X_0",
        "PERF_RES_DRAM_CM_RATIO_UP_X_1",
        "PERF_RES_DRAM_CM_RATIO_UP_X_2",
        "PERF_RES_DRAM_CM_RATIO_UP_X_3",
        "PERF_RES_DRAM_CM_RATIO_UP_X_4",
        "PERF_RES_DRAM_VM_SWAPINESS",
        "PERF_RES_DRAM_VM_DROP_CACHES",
        "PERF_RES_DRAM_VM_WATERMARK_EXTRAKBYTESADJ",
        "PERF_RES_DRAM_VM_WATERMARK_SCALEFACTOR",
        "PERF_RES_DRAM_LMK_KILL_TIMEOUT",
        "PERF_RES_DRAM_LMK_KILL_HEAVIEST_TASK",
        "PERF_RES_DRAM_LMK_RELEASE_MEM",
        "PERF_RES_DRAM_LMK_THRASHING_LIMIT",
        "PERF_RES_DRAM_LMK_SWAP_FREE_LOW_PERCENT",
        "PERF_RES_DRAM_LMK_MINFREE_SCALEFACTOR",
        "PERF_RES_DRAM_DURASPEED_CPUTHRESHOLD",
        "PERF_RES_DRAM_DURASPEED_CPUTARGET",
        "PERF_RES_DRAM_DURASPEED_POLICYLEVEL",
        /* MAJOR = 5 Scheduler*/
        "PERF_RES_SCHED_BOOST_VALUE_ROOT",
        "PERF_RES_SCHED_BOOST_VALUE_FG",
        "PERF_RES_SCHED_BOOST_VALUE_BG",
        "PERF_RES_SCHED_BOOST_VALUE_TA",
        "PERF_RES_SCHED_BOOST_VALUE_RT",
        "PERF_RES_SCHED_PREFER_IDLE_ROOT",
        "PERF_RES_SCHED_PREFER_IDLE_FG",
        "PERF_RES_SCHED_PREFER_IDLE_BG",
        "PERF_RES_SCHED_PREFER_IDLE_TA",
        "PERF_RES_SCHED_PREFER_IDLE_RT",
        "PERF_RES_SCHED_PREFER_IDLE_PER_TASK",
        "PERF_RES_SCHED_PREFER_IDLE_SYS",
        "PERF_RES_SCHED_PREFER_IDLE_SYSBG",
        "PERF_RES_SCHED_UCLAMP_MIN_ROOT",
        "PERF_RES_SCHED_UCLAMP_MIN_FG",
        "PERF_RES_SCHED_UCLAMP_MIN_BG",
        "PERF_RES_SCHED_UCLAMP_MIN_TA",
        "PERF_RES_SCHED_UCLAMP_MIN_RT",
        "PERF_RES_SCHED_UCLAMP_MIN_SYS",
        "PERF_RES_SCHED_UCLAMP_MAX_ROOT",
        "PERF_RES_SCHED_UCLAMP_MAX_FG",
        "PERF_RES_SCHED_UCLAMP_MAX_BG",
        "PERF_RES_SCHED_UCLAMP_MAX_TA",
        "PERF_RES_SCHED_UCLAMP_MAX_RT",
        "PERF_RES_SCHED_UCLAMP_MAX_SYS",
        "PERF_RES_SCHED_UCLAMP_MIN_SYSBG",
        "PERF_RES_SCHED_UCLAMP_MAX_SYSBG",
        "PERF_RES_SCHED_TUNE_THRES",
        "PERF_RES_SCHED_BOOST",
        "PERF_RES_SCHED_MIGRATE_COST",
        "PERF_RES_SCHED_CPU_PREFER_TASK_1_BIG",
        "PERF_RES_SCHED_CPU_PREFER_TASK_1_LITTLE",
        "PERF_RES_SCHED_CPU_PREFER_TASK_1_RESERVED",
        "PERF_RES_SCHED_CPU_PREFER_TASK_2_BIG",
        "PERF_RES_SCHED_CPU_PREFER_TASK_2_LITTLE",
        "PERF_RES_SCHED_CPU_PREFER_TASK_2_RESERVED",
        "PERF_RES_SCHED_BTASK_ROTATE",
        "PERF_RES_SCHED_CACHE_AUDIT",
        "PERF_RES_SCHED_CACHE_PART_CTRL",
        "PERF_RES_SCHED_CACHE_SET_CT_ROOT",
        "PERF_RES_SCHED_CACHE_SET_CT_TA",
        "PERF_RES_SCHED_CACHE_SET_CT_FG",
        "PERF_RES_SCHED_CACHE_SET_CT_BG",
        "PERF_RES_SCHED_CACHE_SET_CT_SYSBG",
        "PERF_RES_SCHED_CACHE_SET_CT_RT",
        "PERF_RES_SCHED_CACHE_SET_CT_SYS",
        "PERF_RES_SCHED_CACHE_SET_CT_TASK",
        "PERF_RES_SCHED_CACHE_SET_NCT_TASK",
        "PERF_RES_SCHED_CACHE_CPUQOS_MODE",
        "PERF_RES_SCHED_PLUS_DOWN_THROTTLE_NS",
        "PERF_RES_SCHED_PLUS_UP_THROTTLE_NS",
        "PERF_RES_SCHED_PLUS_SYNC_FLAG",
        "PERF_RES_SCHED_PLUS_DOWN_THROTTLE_US",
        "PERF_RES_SCHED_PLUS_UP_THROTTLE_US",
        "PERF_RES_SCHED_MTK_PREFER_IDLE",
        "PERF_RES_SCHED_HEAVY_TASK_THRES",
        "PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_AC",
        "PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_THRES",
        "PERF_RES_SCHED_WALT",
        "PERF_RES_SCHED_PREFER_CPU_ROOT",
        "PERF_RES_SCHED_PREFER_CPU_FG",
        "PERF_RES_SCHED_PREFER_CPU_BG",
        "PERF_RES_SCHED_PREFER_CPU_TA",
        "PERF_RES_SCHED_PREFER_CPU_RT",
        "PERF_RES_SCHED_PREFER_CPU_SYSBG",
        "PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_0",
        "PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_1",
        "PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_2",
        "PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_0",
        "PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_1",
        "PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_2",
        "PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_0",
        "PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_1",
        "PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_2",
        "PERF_RES_SCHED_ISOLATION_CPU",
        "PERF_RES_SCHED_CORE_CTL_POLICY_ENABLE",
        "PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_0",
        "PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_1",
        "PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_2",
        "PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_0",
        "PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_1",
        "PERF_RES_SCHED_CPU_CORE_NOT_PREFERRED",
    };
    const std::unordered_map<std::string, int> StringToPowerEnumMap = {
        /* MAJOR = 1 CPU freq*/
        {"PERF_RES_CPUFREQ_MIN_CLUSTER_0", PERF_RES_CPUFREQ_MIN_CLUSTER_0},
        {"PERF_RES_CPUFREQ_MIN_CLUSTER_1", PERF_RES_CPUFREQ_MIN_CLUSTER_1},
        {"PERF_RES_CPUFREQ_MIN_CLUSTER_2", PERF_RES_CPUFREQ_MIN_CLUSTER_2},
        {"PERF_RES_CPUFREQ_MAX_CLUSTER_0", PERF_RES_CPUFREQ_MAX_CLUSTER_0},
        {"PERF_RES_CPUFREQ_MAX_CLUSTER_1", PERF_RES_CPUFREQ_MAX_CLUSTER_1},
        {"PERF_RES_CPUFREQ_MAX_CLUSTER_2", PERF_RES_CPUFREQ_MAX_CLUSTER_2},
       {"PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0", PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0},
        {"PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1", PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1},
        {"PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2", PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2},
        {"PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0", PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0},
        {"PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1", PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1},
        {"PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2", PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2},
        {"PERF_RES_CPUFREQ_CCI_FREQ", PERF_RES_CPUFREQ_CCI_FREQ},
        {"PERF_RES_CPUFREQ_PERF_MODE", PERF_RES_CPUFREQ_PERF_MODE},
        /* MAJOR = 2 CPU core*/
        {"PERF_RES_CPUCORE_MIN_CLUSTER_0", PERF_RES_CPUCORE_MIN_CLUSTER_0},
        {"PERF_RES_CPUCORE_MIN_CLUSTER_1", PERF_RES_CPUCORE_MIN_CLUSTER_1},
        {"PERF_RES_CPUCORE_MIN_CLUSTER_2", PERF_RES_CPUCORE_MIN_CLUSTER_2},
        {"PERF_RES_CPUCORE_MAX_CLUSTER_0", PERF_RES_CPUCORE_MAX_CLUSTER_0},
        {"PERF_RES_CPUCORE_MAX_CLUSTER_1", PERF_RES_CPUCORE_MAX_CLUSTER_1},
        {"PERF_RES_CPUCORE_MAX_CLUSTER_2", PERF_RES_CPUCORE_MAX_CLUSTER_2},
        {"PERF_RES_CPUCORE_PERF_MODE", PERF_RES_CPUCORE_PERF_MODE},
        {"PERF_RES_CPUCORE_FORCE_PAUSE_CPU", PERF_RES_CPUCORE_FORCE_PAUSE_CPU},
        /* MAJOR = 3 GPU*/
        {"PERF_RES_GPU_FREQ_MIN", PERF_RES_GPU_FREQ_MIN},
        {"PERF_RES_GPU_FREQ_MAX", PERF_RES_GPU_FREQ_MAX},
        {"PERF_RES_GPU_FREQ_LOW_LATENCY", PERF_RES_GPU_FREQ_LOW_LATENCY},
        {"PERF_RES_GPU_GED_MARGIN_MODE", PERF_RES_GPU_GED_MARGIN_MODE},
        {"PERF_RES_GPU_GED_TIMER_BASE_DVFS_MARGIN", PERF_RES_GPU_GED_TIMER_BASE_DVFS_MARGIN},
        {"PERF_RES_GPU_GED_LOADING_BASE_DVFS_STEP", PERF_RES_GPU_GED_LOADING_BASE_DVFS_STEP},
        {"PERF_RES_GPU_GED_CWAITG", PERF_RES_GPU_GED_CWAITG},
        {"PERF_RES_GPU_GED_GX_BOOST", PERF_RES_GPU_GED_GX_BOOST},
        {"PERF_RES_GPU_GED_DVFS_LOADING_MODE", PERF_RES_GPU_GED_DVFS_LOADING_MODE},
        {"PERF_RES_GPU_POWER_POLICY", PERF_RES_GPU_POWER_POLICY},
        {"PERF_RES_GPU_POWER_ONOFF_INTERVAL", PERF_RES_GPU_POWER_ONOFF_INTERVAL},
        /* MAJOR = 4 DRAM (VCORE, CM MGR, EMI)*/
        {"PERF_RES_DRAM_OPP_MIN", PERF_RES_DRAM_OPP_MIN},
        {"PERF_RES_DRAM_OPP_MIN_LP5", PERF_RES_DRAM_OPP_MIN_LP5},
        {"PERF_RES_DRAM_OPP_MIN_LP5_HFR", PERF_RES_DRAM_OPP_MIN_LP5_HFR},
        {"PERF_RES_DRAM_VCORE_MIN", PERF_RES_DRAM_VCORE_MIN},
        {"PERF_RES_DRAM_VCORE_MIN_LP3", PERF_RES_DRAM_VCORE_MIN_LP3},
        {"PERF_RES_DRAM_VCORE_BW_ENABLE", PERF_RES_DRAM_VCORE_BW_ENABLE},
        {"PERF_RES_DRAM_VCORE_BW_THRES", PERF_RES_DRAM_VCORE_BW_THRES},
        {"PERF_RES_DRAM_VCORE_BW_THRESH_LP3", PERF_RES_DRAM_VCORE_BW_THRESH_LP3},
        {"PERF_RES_DRAM_VCORE_POLICY", PERF_RES_DRAM_VCORE_POLICY},
        {"PERF_RES_DRAM_CM_MGR", PERF_RES_DRAM_CM_MGR},
        {"PERF_RES_DRAM_CM_MGR_CAM_ENABLE", PERF_RES_DRAM_CM_MGR_CAM_ENABLE},
        {"PERF_RES_DRAM_CM_RATIO_UP_X_0", PERF_RES_DRAM_CM_RATIO_UP_X_0},
        {"PERF_RES_DRAM_CM_RATIO_UP_X_1", PERF_RES_DRAM_CM_RATIO_UP_X_1},
        {"PERF_RES_DRAM_CM_RATIO_UP_X_2", PERF_RES_DRAM_CM_RATIO_UP_X_2},
        {"PERF_RES_DRAM_CM_RATIO_UP_X_3", PERF_RES_DRAM_CM_RATIO_UP_X_3},
        {"PERF_RES_DRAM_CM_RATIO_UP_X_4", PERF_RES_DRAM_CM_RATIO_UP_X_4},
        {"PERF_RES_DRAM_VM_SWAPINESS", PERF_RES_DRAM_VM_SWAPINESS},
        {"PERF_RES_DRAM_VM_DROP_CACHES", PERF_RES_DRAM_VM_DROP_CACHES},
        {"PERF_RES_DRAM_VM_WATERMARK_EXTRAKBYTESADJ", PERF_RES_DRAM_VM_WATERMARK_EXTRAKBYTESADJ},
        {"PERF_RES_DRAM_VM_WATERMARK_SCALEFACTOR", PERF_RES_DRAM_VM_WATERMARK_SCALEFACTOR},
        {"PERF_RES_DRAM_LMK_KILL_TIMEOUT", PERF_RES_DRAM_LMK_KILL_TIMEOUT},
        {"PERF_RES_DRAM_LMK_KILL_HEAVIEST_TASK", PERF_RES_DRAM_LMK_KILL_HEAVIEST_TASK},
        {"PERF_RES_DRAM_LMK_RELEASE_MEM", PERF_RES_DRAM_LMK_RELEASE_MEM},
        {"PERF_RES_DRAM_LMK_THRASHING_LIMIT",PERF_RES_DRAM_LMK_THRASHING_LIMIT},
        {"PERF_RES_DRAM_LMK_SWAP_FREE_LOW_PERCENT", PERF_RES_DRAM_LMK_SWAP_FREE_LOW_PERCENT},
        {"PERF_RES_DRAM_LMK_MINFREE_SCALEFACTOR", PERF_RES_DRAM_LMK_MINFREE_SCALEFACTOR},
        {"PERF_RES_DRAM_DURASPEED_CPUTHRESHOLD", PERF_RES_DRAM_DURASPEED_CPUTHRESHOLD},
        {"PERF_RES_DRAM_DURASPEED_CPUTARGET", PERF_RES_DRAM_DURASPEED_CPUTARGET},
        {"PERF_RES_DRAM_DURASPEED_POLICYLEVEL", PERF_RES_DRAM_DURASPEED_POLICYLEVEL},
        /* MAJOR = 5 Scheduler*/
        {"PERF_RES_SCHED_BOOST_VALUE_ROOT", PERF_RES_SCHED_BOOST_VALUE_ROOT},
        {"PERF_RES_SCHED_BOOST_VALUE_FG", PERF_RES_SCHED_BOOST_VALUE_FG},
        {"PERF_RES_SCHED_BOOST_VALUE_BG", PERF_RES_SCHED_BOOST_VALUE_BG},
        {"PERF_RES_SCHED_BOOST_VALUE_TA", PERF_RES_SCHED_BOOST_VALUE_TA},
        {"PERF_RES_SCHED_BOOST_VALUE_RT", PERF_RES_SCHED_BOOST_VALUE_RT},
        {"PERF_RES_SCHED_PREFER_IDLE_ROOT", PERF_RES_SCHED_PREFER_IDLE_ROOT},
        {"PERF_RES_SCHED_PREFER_IDLE_FG", PERF_RES_SCHED_PREFER_IDLE_FG},
        {"PERF_RES_SCHED_PREFER_IDLE_BG", PERF_RES_SCHED_PREFER_IDLE_BG},
        {"PERF_RES_SCHED_PREFER_IDLE_TA", PERF_RES_SCHED_PREFER_IDLE_TA},
        {"PERF_RES_SCHED_PREFER_IDLE_RT", PERF_RES_SCHED_PREFER_IDLE_RT},
        {"PERF_RES_SCHED_PREFER_IDLE_PER_TASK", PERF_RES_SCHED_PREFER_IDLE_PER_TASK},
        {"PERF_RES_SCHED_PREFER_IDLE_SYS", PERF_RES_SCHED_PREFER_IDLE_SYS},
        {"PERF_RES_SCHED_PREFER_IDLE_SYSBG", PERF_RES_SCHED_PREFER_IDLE_SYSBG},
        {"PERF_RES_SCHED_UCLAMP_MIN_ROOT", PERF_RES_SCHED_UCLAMP_MIN_ROOT},
        {"PERF_RES_SCHED_UCLAMP_MIN_FG", PERF_RES_SCHED_UCLAMP_MIN_FG},
        {"PERF_RES_SCHED_UCLAMP_MIN_BG", PERF_RES_SCHED_UCLAMP_MIN_BG},
        {"PERF_RES_SCHED_UCLAMP_MIN_TA", PERF_RES_SCHED_UCLAMP_MIN_TA},
        {"PERF_RES_SCHED_UCLAMP_MIN_RT", PERF_RES_SCHED_UCLAMP_MIN_RT},
        {"PERF_RES_SCHED_UCLAMP_MIN_SYS", PERF_RES_SCHED_UCLAMP_MIN_SYS},
        {"PERF_RES_SCHED_UCLAMP_MAX_ROOT", PERF_RES_SCHED_UCLAMP_MAX_ROOT},
        {"PERF_RES_SCHED_UCLAMP_MAX_FG", PERF_RES_SCHED_UCLAMP_MAX_FG},
        {"PERF_RES_SCHED_UCLAMP_MAX_BG", PERF_RES_SCHED_UCLAMP_MAX_BG},
        {"PERF_RES_SCHED_UCLAMP_MAX_TA", PERF_RES_SCHED_UCLAMP_MAX_TA},
        {"PERF_RES_SCHED_UCLAMP_MAX_RT", PERF_RES_SCHED_UCLAMP_MAX_RT},
        {"PERF_RES_SCHED_UCLAMP_MAX_SYS", PERF_RES_SCHED_UCLAMP_MAX_SYS},
        {"PERF_RES_SCHED_UCLAMP_MIN_SYSBG", PERF_RES_SCHED_UCLAMP_MIN_SYSBG},
        {"PERF_RES_SCHED_UCLAMP_MAX_SYSBG", PERF_RES_SCHED_UCLAMP_MAX_SYSBG},
        {"PERF_RES_SCHED_TUNE_THRES", PERF_RES_SCHED_TUNE_THRES},
        {"PERF_RES_SCHED_BOOST", PERF_RES_SCHED_BOOST},
        {"PERF_RES_SCHED_MIGRATE_COST", PERF_RES_SCHED_MIGRATE_COST},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_1_BIG", PERF_RES_SCHED_CPU_PREFER_TASK_1_BIG},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_1_LITTLE", PERF_RES_SCHED_CPU_PREFER_TASK_1_LITTLE},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_1_RESERVED", PERF_RES_SCHED_CPU_PREFER_TASK_1_RESERVED},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_2_BIG", PERF_RES_SCHED_CPU_PREFER_TASK_2_BIG},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_2_LITTLE", PERF_RES_SCHED_CPU_PREFER_TASK_2_LITTLE},
        {"PERF_RES_SCHED_CPU_PREFER_TASK_2_RESERVED", PERF_RES_SCHED_CPU_PREFER_TASK_2_RESERVED},
        {"PERF_RES_SCHED_BTASK_ROTATE", PERF_RES_SCHED_BTASK_ROTATE},
        {"PERF_RES_SCHED_CACHE_AUDIT", PERF_RES_SCHED_CACHE_AUDIT},
        {"PERF_RES_SCHED_CACHE_PART_CTRL", PERF_RES_SCHED_CACHE_PART_CTRL},
        {"PERF_RES_SCHED_CACHE_SET_CT_ROOT", PERF_RES_SCHED_CACHE_SET_CT_ROOT},
        {"PERF_RES_SCHED_CACHE_SET_CT_TA", PERF_RES_SCHED_CACHE_SET_CT_TA},
        {"PERF_RES_SCHED_CACHE_SET_CT_FG", PERF_RES_SCHED_CACHE_SET_CT_FG},
        {"PERF_RES_SCHED_CACHE_SET_CT_BG", PERF_RES_SCHED_CACHE_SET_CT_BG},
        {"PERF_RES_SCHED_CACHE_SET_CT_SYSBG", PERF_RES_SCHED_CACHE_SET_CT_SYSBG},
        {"PERF_RES_SCHED_CACHE_SET_CT_RT", PERF_RES_SCHED_CACHE_SET_CT_RT},
        {"PERF_RES_SCHED_CACHE_SET_CT_SYS", PERF_RES_SCHED_CACHE_SET_CT_SYS},
        {"PERF_RES_SCHED_CACHE_SET_CT_TASK", PERF_RES_SCHED_CACHE_SET_CT_TASK},
        {"PERF_RES_SCHED_CACHE_SET_NCT_TASK", PERF_RES_SCHED_CACHE_SET_NCT_TASK},
        {"PERF_RES_SCHED_CACHE_CPUQOS_MODE", PERF_RES_SCHED_CACHE_CPUQOS_MODE},
        {"PERF_RES_SCHED_PLUS_DOWN_THROTTLE_NS", PERF_RES_SCHED_PLUS_DOWN_THROTTLE_NS},
        {"PERF_RES_SCHED_PLUS_UP_THROTTLE_NS", PERF_RES_SCHED_PLUS_UP_THROTTLE_NS},
        {"PERF_RES_SCHED_PLUS_SYNC_FLAG", PERF_RES_SCHED_PLUS_SYNC_FLAG},
        {"PERF_RES_SCHED_PLUS_DOWN_THROTTLE_US", PERF_RES_SCHED_PLUS_DOWN_THROTTLE_US},
        {"PERF_RES_SCHED_PLUS_UP_THROTTLE_US", PERF_RES_SCHED_PLUS_UP_THROTTLE_US},
        {"PERF_RES_SCHED_MTK_PREFER_IDLE", PERF_RES_SCHED_MTK_PREFER_IDLE},
        {"PERF_RES_SCHED_HEAVY_TASK_THRES", PERF_RES_SCHED_HEAVY_TASK_THRES},
        {"PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_AC", PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_AC},
        {"PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_THRES", PERF_RES_SCHED_HEAVY_TASK_AVG_HTASK_THRES},
        {"PERF_RES_SCHED_WALT", PERF_RES_SCHED_WALT},
        {"PERF_RES_SCHED_PREFER_CPU_ROOT", PERF_RES_SCHED_PREFER_CPU_ROOT},
        {"PERF_RES_SCHED_PREFER_CPU_FG", PERF_RES_SCHED_PREFER_CPU_FG},
        {"PERF_RES_SCHED_PREFER_CPU_BG", PERF_RES_SCHED_PREFER_CPU_BG},
        {"PERF_RES_SCHED_PREFER_CPU_TA", PERF_RES_SCHED_PREFER_CPU_TA},
        {"PERF_RES_SCHED_PREFER_CPU_RT", PERF_RES_SCHED_PREFER_CPU_RT},
        {"PERF_RES_SCHED_PREFER_CPU_SYSBG", PERF_RES_SCHED_PREFER_CPU_SYSBG},
        {"PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_0", PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_0},
        {"PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_1", PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_1},
        {"PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_2", PERF_RES_SCHED_UTIL_RATE_LIMIT_US_CLUSTER_2},
        {"PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_0", PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_0},
        {"PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_1", PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_1},
        {"PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_2", PERF_RES_SCHED_UTIL_UP_RATE_LIMIT_US_CLUSTER_2},
        {"PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_0", PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_0},
        {"PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_1", PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_1},
        {"PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_2", PERF_RES_SCHED_UTIL_DOWN_RATE_LIMIT_US_CLUSTER_2},
        {"PERF_RES_SCHED_ISOLATION_CPU", PERF_RES_SCHED_ISOLATION_CPU},
        {"PERF_RES_SCHED_CORE_CTL_POLICY_ENABLE", PERF_RES_SCHED_CORE_CTL_POLICY_ENABLE},
        {"PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_0", PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_0},
        {"PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_1", PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_1},
        {"PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_2", PERF_RES_SCHED_OFFLINE_THROTTLE_MS_CLUSTER_2},
        {"PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_0", PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_0},
        {"PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_1", PERF_RES_SCHED_BTASK_UP_THRESH_CLUSTER_1},
        {"PERF_RES_SCHED_CPU_CORE_NOT_PREFERRED", PERF_RES_SCHED_CPU_CORE_NOT_PREFERRED},
    };
};
#endif