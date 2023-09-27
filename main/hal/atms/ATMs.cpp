#include "ATMs.h"
#include <log/log.h>
#include <cutils/properties.h>
#include <ATMsEventMgr.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>

 using std::string;
 static  std::map<std::string, std::string> mMap;

namespace vendor {
namespace mediatek {
namespace hardware {
namespace camera {
namespace atms {
namespace V1_0 {
namespace implementation {

using namespace android;

// Methods from ::vendor::mediatek::hardware::camera::atms::V1_0::IATMs follow.
Return<int32_t> ATMs::set_atms_factor(const hidl_string& factor_type, const hidl_string& factor_value) {
    ALOGD("[HIDL][ATMs]Received factor(%s), factor_value(%s)", factor_type.c_str(), factor_value.c_str());
    std::string str_factor_type("");
    std::string str_factor_value("");
    const char* tmp_type = factor_type.c_str();
    if(tmp_type != nullptr){
        str_factor_type = tmp_type;
    }else{
        ALOGD("[%s] factor_type.c_str() is null, set str_factor_type to empty.", __FUNCTION__);
    }
    const char* tmp_value = factor_value.c_str();
    if(tmp_value != nullptr){
        str_factor_value = tmp_value;
    }else{
        ALOGD("[%s] factor_value.c_str() is null, set str_factor_value to empty.", __FUNCTION__);
    }
    ATMsWrap::getInstance()->setFactor(str_factor_type, str_factor_value);
    return int32_t {};
}

Return<void> ATMs::get_atms_factor(const hidl_string& factor_type, get_atms_factor_cb _hidl_cb) {
    std::string str_factor_type("");
    const char* tmp_type = factor_type.c_str();
    if(tmp_type != nullptr){
       str_factor_type = tmp_type;
    }else{
       ALOGD("[%s] factor_type.c_str() is null, set str_factor_type to empty.", __FUNCTION__);
    }
    _hidl_cb(str_factor_type);
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IATMs* HIDL_FETCH_IATMs(const char* /* name */) {
    return new ATMs();
}

/* ATMsWrap */
/*******************************************************************************
* Get Instance
********************************************************************************/
static std::once_flag onceFlag;
static std::unique_ptr<ATMsWrap> instance;

ATMsWrap*
ATMsWrap::getInstance()
{
    std::call_once(onceFlag, [&] {
        instance = std::make_unique<ATMsWrap>();
    } );
    return instance.get();
}

/******************************************************************************
 *
 ******************************************************************************/
Return<int32_t>
ATMsWrap::setFactor(string factor_type, string factor_Value) {
    Mutex::Autolock _l(mLock);
    NSCam::ATMsEventMgr::getInstance()->setFactor(factor_type, factor_Value);

    return int32_t {};
}

//
}  // namespace implementation
}  // namespace V1_0
}  // namespace atms
}  // namespace camera
}  // namespace hardware
}  // namespace mediatek
}  // namespace vendor
