LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

SOURCE_FILES := $(wildcard $(LOCAL_PATH)/meta/*.cpp)
SOURCE_FILES += $(wildcard $(LOCAL_PATH)/operator/*.cpp)
SOURCE_FILES += $(wildcard $(LOCAL_PATH)/utils/*.cpp)
SOURCE_FILES += $(wildcard $(LOCAL_PATH)/data/*.cpp)
SOURCE_FILES += $(wildcard $(LOCAL_PATH)/parser/*.cpp)
SOURCE_FILES += $(wildcard $(LOCAL_PATH)/packer/*.cpp)
SOURCE_FILES += StereoInfoAccessor.cpp

LOCAL_SRC_FILES := $(SOURCE_FILES:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES +=                                     \
    $(LOCAL_PATH)/                                      \
    $(TOP)/vendor/mediatek/opensource/external/XMP-Toolkit-SDK/                              \
    $(TOP)/vendor/mediatek/opensource/external/XMP-Toolkit-SDK/public/include                \
    $(TOP)/vendor/mediatek/opensource/external/XMP-Toolkit-SDK/public/include/client-glue    \
    $(TOP)/vendor/mediatek/opensource/external/XMP-Toolkit-SDK/third-party/zuid/interfaces   \
    $(LOCAL_PATH)/meta/                                 \
    $(LOCAL_PATH)/meta/data/                            \
    $(LOCAL_PATH)/operator/                             \
    $(LOCAL_PATH)/utils/                                \
    $(LOCAL_PATH)/data/                                 \
    $(LOCAL_PATH)/parser/                               \
    $(LOCAL_PATH)/packer/                               \
    $(TOP)/vendor/mediatek/opensource/external/rapidjson/include/rapidjson    \
    $(TOP)/external/libpng/

LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/hardware/mtkcam3/include/

LOCAL_CFLAGS += -DHAVE_MEMMOVE -DUNIX_ENV -DSTDC -fexceptions

APP_STL := stlport_shared

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexpat libz libpng

LOCAL_STATIC_LIBRARIES := libxmp_vsdof

LOCAL_MODULE := libstereoinfoaccessor_vsdof
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTKCAM_SHARED_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))
