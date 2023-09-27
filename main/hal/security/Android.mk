LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# securecamera_test - Unit tests of secure camera device
LOCAL_MODULE := securecamera_test
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

LOCAL_SRC_FILES := \
	tests/InteractiveTest.cpp \
	tests/Camera2VendorTest.cpp \
	tests/MapperAndAllocatorTest.cpp \
	tests/ImageBufferHeapTest.cpp \
	tests/main.cpp

LOCAL_HEADER_LIBRARIES := \
	libnativebase_headers \
	libui_headers \
	libmtkcam_headers \
  libgralloc_metadata_headers

LOCAL_STATIC_LIBRARIES := \
	libarect \
	libsecurecameratest_legacy \
	android.hardware.camera.common@1.0-helper \
	libdmabufheap

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libsync \
	libutils \
	libcutils \
	libgralloc_extra \
	libnativewindow \
	libcamera_metadata \
	libmediandk \
	libcamera2ndk_vendor \
	libhidlbase \
	android.hardware.graphics.mapper@4.0 \
	android.hardware.graphics.allocator@4.0 \
	libmtkcam_ulog \
	libmtkcam_imgbuf \
	libmtkcam.debugwrapper

# NOTE: set ENABLE_SECURE_IMAGE_DATA_TEST to false for
#       testing normal camera flow with Camera HAL3 impl.
#       This compiler flag is a handy option for
#       comparing the difference between normal and secure flow.
LOCAL_CPPFLAGS := -DENABLE_SECURE_IMAGE_DATA_TEST=true

# We depends LL-NDK API AImageReader_getWindowNativeHandle of the Vendor NDK,
# so add __ANDROID_VNDK__ to enable this LL-NDK API.
#
# Reference:
# https://source.android.com/devices/architecture/vndk/enabling#managing-headers
LOCAL_CPPFLAGS += -D__ANDROID_VNDK__

LOCAL_CPPFLAGS += -Werror

# MTK TEE
ifeq ($(strip $(MTK_ENABLE_GENIEZONE)), yes)
$(info MTEE is enabled)
LOCAL_CFLAGS += -DMTK_MTEE_SUPPORTED=true
else
LOCAL_CFLAGS += -DMTK_MTEE_SUPPORTED=false
endif

ifeq ($(strip $(MTK_CAM_GENIEZONE_SUPPORT)), yes)
LOCAL_CFLAGS += -DMTKCAM_MTEE_PATH_SUPPORTED=true
else
LOCAL_CFLAGS += -DMTKCAM_MTEE_PATH_SUPPORTED=false
endif # MTK_CAM_GENIEZONE_SUPPORT


include $(MTKCAM_BUILD_NATIVE_TEST)